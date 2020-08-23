#if defined(_MSC_VER)
#pragma warning ( disable : 4786 )
#pragma warning ( disable : 4996 )
#endif

#include "utilities.hpp"
#include "json.hpp"
#include "smartptr.hpp"
#include "strings.hpp"
#include "filesys.hpp"
#include "time.hpp"
#include <fstream>
#include <algorithm>
#include <iomanip>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>

#if defined(_MSC_VER) || defined(WIN32)

#include <io.h>
#include <process.h>

#ifdef __GNUC__ // for mingw
_CRTIMP int __cdecl __MINGW_NOTHROW _stricmp (const char*, const char*);
_CRTIMP int __cdecl __MINGW_NOTHROW swprintf (wchar_t*, const wchar_t*, ...);
_CRTIMP int __cdecl __MINGW_NOTHROW _wcsicmp (const wchar_t*, const wchar_t*);
#endif

#else

#include <unistd.h>
#include <errno.h>

// linux别名
#define _stricmp strcasecmp
#define _close close
#define _open open
#define _read read
#define _write write
#define _O_RDONLY O_RDONLY
#define _O_CREAT O_CREAT
#define _O_TRUNC O_TRUNC
#define _O_WRONLY O_WRONLY
#define _O_TEXT 0
#if defined(S_IREAD) && defined(S_IWRITE)
#define _S_IREAD S_IREAD
#define _S_IWRITE S_IWRITE
#else
#define _S_IREAD S_IRUSR
#define _S_IWRITE S_IWUSR
#endif
#define _wcsicmp wcscasecmp

#endif

namespace winux
{

WINUX_FUNC_IMPL(bool) ValueIsInArray( StringArray const & arr, String const & val, bool caseInsensitive )
{
    int (*pStrCmp)( char const *, char const * ) = caseInsensitive ? &_stricmp: &strcmp;
    StringArray::const_iterator it;
    for ( it = arr.begin(); it != arr.end(); ++it )
    {
        if ( (*pStrCmp)( (*it).c_str(), val.c_str() ) == 0 )
        {
            return true;
        }
    }
    return false;
}

WINUX_FUNC_IMPL(String) FileGetContents( String const & filename )
{
    String content;
    try
    {
        SimpleHandle<int> fd( _open( filename.c_str(), _O_RDONLY | _O_TEXT ), -1, _close );
        if ( fd )
        {
            int readBytes = 0, currRead = 0;
            char buf[4096];
            do
            {
                memset( buf, 0, 4096 );
                if ( ( currRead = _read( fd.get(), buf, 4096 ) ) < 1 ) break;
                content.append( buf, currRead );
                readBytes += currRead;
            } while ( currRead > 0 );
        }
    }
    catch ( std::exception const & )
    {
    }
    return content;
}

WINUX_FUNC_IMPL(bool) FilePutContents( String const & filename, String const & str )
{
    bool r = false;
    try
    {
        SimpleHandle<int> fd(
            _open(
                filename.c_str(),
                _O_CREAT | _O_TRUNC | _O_WRONLY | _O_TEXT,
            #if defined(_MSC_VER) || defined(WIN32)
                _S_IREAD | _S_IWRITE
            #else
                S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH
            #endif
            ),
            -1,
            _close
        );
        if ( fd )
        {
            int writtenBytes = _write( fd.get(), str.c_str(), str.length() );
            if ( writtenBytes == (int)str.length() )
                r = true;
        }
        /*else
        {
            switch ( errno )
            {
            case EACCES:
                printf("Tried to open read-only file for writing, file's sharing mode does not allow specified operations, or given path is directory.");
                break;
            case EEXIST:
                printf("_O_CREAT and _O_EXCL flags specified, but filename already exists.");
                break;
            }
        }*/
    }
    catch ( std::exception const & )
    {
    }
    return r;
}

WINUX_FUNC_IMPL(int) Random( int n1, int n2 )
{
    static int seedInit = VoidReturnInt( srand, (unsigned int)GetUtcTimeMs() );
    (void)seedInit;
    return abs( rand() * rand() ) % ( abs( n2 - n1 ) + 1 ) + ( n1 < n2 ? 1 : -1 ) * n1;
}

WINUX_FUNC_IMPL(void) WriteLog( String const & s )
{
    String exeFile;
    String exePath = FilePath( GetExecutablePath(), &exeFile );
    std::ofstream out( ( exePath + dirSep + FileTitle(exeFile) + ".log" ).c_str(), std::ios_base::out | std::ios_base::app );
    //_getpid();
    time_t tt = time(NULL);
    struct tm * t = gmtime(&tt);
    char sz[32] = "";
    strftime( sz, 32, "%a, %d %b %Y %H:%M:%S GMT", t );

    out << Format( "[pid:%d]", getpid() ) << sz << " - " << AddCSlashes(s) << std::endl;
}

WINUX_FUNC_IMPL(void) WriteBinLog( void const* data, int size )
{
    String exeFile;
    String exePath = FilePath( GetExecutablePath(), &exeFile );
    std::ofstream out( ( exePath + dirSep + FileTitle(exeFile) + ".binlog" ).c_str(), std::ios_base::out | std::ios_base::binary | std::ios_base::app );
    out.write( (char const *)data, size );
}

////////////////////////////////////////////////////////////////////////////////////////////////
// class Configure -----------------------------------------------------------------------------
Configure::Configure()
{

}

Configure::Configure( String const & configFile )
{
    this->load(configFile);
}

int Configure::_FindConfigRef( String const & str, int offset, int * length, String * name )
{
    String ldelim = "$(";
    String rdelim = ")";
    *length = 0;
    int pos1 = (int)str.find( ldelim, offset );
    if ( pos1 == -1 ) return -1;
    int pos2 = (int)str.find( rdelim, pos1 + ldelim.length() );
    if ( pos2 == -1 ) return -1;
    *length = pos2 + rdelim.length() - pos1;
    *name = str.substr( pos1 + ldelim.length(), pos2 - pos1 - ldelim.length() );
    return pos1;
}

String Configure::_expandVarNoStripSlashes( String const & name, StringArray * chains ) const
{
    if ( !this->has(name) ) return "";
    chains->push_back(name);
    String configVal = _rawParams.at(name);
    String res = "";
    int len;
    String varName;
    int offset = 0;
    int pos;
    while ( ( pos = _FindConfigRef( configVal, offset, &len, &varName ) ) != -1 )
    {
        res += configVal.substr( offset, pos - offset );
        offset = pos + len;
        res += !ValueIsInArray( *chains, varName ) ? _expandVarNoStripSlashes( varName, chains ) : "";
    }
    res += configVal.substr(offset);
    chains->pop_back();
    return res;
}

int Configure::_load( String const & configFile, StringStringMap * rawParams, StringArray * loadFileChains )
{
    loadFileChains->push_back( RealPath(configFile) );
    int lineCount = 0;
    int varsCount = 0;
    try
    {
        std::ifstream fin( configFile.c_str() );
        String line;
        while ( std::getline( fin, line ) )
        {
            lineCount++;
            String tmp = StrTrim(line);

            if ( tmp.empty() || tmp[0] == '#' ) // '#'行开头表示注释,只有在行开头才表示注释
                continue;

            if ( tmp[0] == '@' ) // '@'开头表示配置命令,目前仅支持 @import other-config-file
            {
                int pos = (int)tmp.find(' ');
                String commandName, commandParam;
                if ( pos == -1 )
                {
                    commandName = tmp.substr(1); // 偏移1是为了 skip '@'
                    commandParam = "";
                }
                else
                {
                    commandName = tmp.substr( 1, pos - 1 );
                    commandParam = tmp.substr( pos + 1 );
                }
                if ( commandName == "import" ) // 导入外部配置
                {
                    String dirPath = FilePath(configFile);
                    String confPath = commandParam;
                    confPath = IsAbsPath(confPath) ? confPath : ( dirPath.empty() ? "" : dirPath + dirSep ) + confPath;
                    if ( !ValueIsInArray( *loadFileChains, RealPath(confPath), true ) )
                    {
                        varsCount += _load( confPath, rawParams, loadFileChains );
                    }
                }
                continue;
            }

            int delimPos = (int)line.find('=');
            if ( delimPos == -1 ) // 找不到'='分隔符,就把整行当成参数名
                (*rawParams)[line] = "";
            else
                (*rawParams)[ line.substr( 0, delimPos ) ] = line.substr( delimPos + 1 );

            varsCount++;
        }
    }
    catch ( std::exception & )
    {
    }
    //loadFileChains->pop_back(); //注释掉这句，则每一个文件只载入一次
    return varsCount;
}

int Configure::load( String const & configFile )
{
    _configFile = configFile;
    StringArray loadFileChains;
    return _load( configFile, &_rawParams, &loadFileChains );
}

#define CONFIG_VARS_SLASH_CHARS "\n\r\t\v\a\\\'\"$()"

String Configure::get( String const & name, bool stripslashes, bool expand ) const
{
    String value;

    if ( expand )
    {
        StringArray callChains; // 递归调用链，防止无穷递归
        value = _expandVarNoStripSlashes( name, &callChains );
    }
    else
    {
        value = this->has(name) ? _rawParams.at(name) : "";
    }

    if ( stripslashes )
        value = StripSlashes( value, CONFIG_VARS_SLASH_CHARS );

    return value;
}

String Configure::operator [] ( String const & name ) const
{
    return this->has(name) ? StripSlashes( _rawParams.at(name), CONFIG_VARS_SLASH_CHARS ) : "";
}

String Configure::operator () ( String const & name ) const
{
    StringArray callChains; // 递归调用链，防止无穷递归
    return StripSlashes( _expandVarNoStripSlashes( name, &callChains ), CONFIG_VARS_SLASH_CHARS );
}

void Configure::setRaw( String const & name, String const & value )
{
    _rawParams[name] = value;
}

void Configure::set( String const & name, String const & value )
{
    _rawParams[name] = AddSlashes( value, CONFIG_VARS_SLASH_CHARS );
}

bool Configure::del( String const & name )
{
    if ( this->has(name) )
    {
        _rawParams.erase(name);
        return true;
    }
    return false;
}

void Configure::clear()
{
    _rawParams.clear();
}

// -------------------------------------------------------------------------------------------------
/* flag values */
#define FL_UNSIGNED   1       /* strtouq called */
#define FL_NEG        2       /* negative sign found */
#define FL_OVERFLOW   4       /* overflow occured */
#define FL_READDIGIT  8       /* we've read at least one correct digit */

#if defined(__GNUC__) /*&& !defined(WIN32)*/
#define _UI64_MAX 0xffffffffffffffffull
#define _I64_MAX 9223372036854775807ll
#define _I64_MIN (-9223372036854775807ll - 1ll)
#endif

static uint64 __StrToXQ( char const * nptr, char const * * endptr, int ibase, int flags )
{
    char const * p;
    char c;
    uint64 number;
    uint digval;
    uint64 maxval;

    p = nptr;            /* p is our scanning pointer */
    number = 0;            /* start with zero */

    c = *p++;            /* read char */

    while ( isspace((int)(unsigned char)c) )
        c = *p++;        /* skip whitespace */

    if (c == '-') {
        flags |= FL_NEG;    /* remember minus sign */
        c = *p++;
    }
    else if (c == '+')
        c = *p++;        /* skip sign */

    if (ibase < 0 || ibase == 1 || ibase > 36) {
        /* bad base! */
        if (endptr)
            /* store beginning of string in endptr */
            *endptr = nptr;
        return 0L;        /* return 0 */
    }
    else if (ibase == 0) {
        /* determine base free-lance, based on first two chars of
           string */
        if (c != '0')
            ibase = 10;
        else if (*p == 'x' || *p == 'X')
            ibase = 16;
        else
            ibase = 8;
    }

    if (ibase == 16) {
        /* we might have 0x in front of number; remove if there */
        if (c == '0' && (*p == 'x' || *p == 'X')) {
            ++p;
            c = *p++;    /* advance past prefix */
        }
    }

    /* if our number exceeds this, we will overflow on multiply */
    maxval = _UI64_MAX / ibase;


    for (;;) {    /* exit in middle of loop */
        /* convert c to value */
        if ( isdigit((int)(unsigned char)c) )
            digval = c - '0';
        else if ( isalpha((int)(unsigned char)c) )
            digval = toupper(c) - 'A' + 10;
        else
            break;
        if (digval >= (unsigned)ibase)
            break;        /* exit loop if bad digit found */

        /* record the fact we have read one digit */
        flags |= FL_READDIGIT;

        /* we now need to compute number = number * base + digval,
           but we need to know if overflow occured.  This requires
           a tricky pre-check. */

        if (number < maxval || (number == maxval &&
        (uint64)digval <= _UI64_MAX % ibase)) {
            /* we won't overflow, go ahead and multiply */
            number = number * ibase + digval;
        }
        else {
            /* we would have overflowed -- set the overflow flag */
            flags |= FL_OVERFLOW;
        }

        c = *p++;        /* read next digit */
    }

    --p;                /* point to place that stopped scan */

    if (!(flags & FL_READDIGIT)) {
        /* no number there; return 0 and point to beginning of
           string */
        if (endptr)
            /* store beginning of string in endptr later on */
            p = nptr;
        number = 0L;        /* return 0 */
    }
    else if ( (flags & FL_OVERFLOW) ||
              ( !(flags & FL_UNSIGNED) &&
                ( ( (flags & FL_NEG) && (number > -_I64_MIN) ) ||
                  ( !(flags & FL_NEG) && (number > _I64_MAX) ) ) ) )
    {
        /* overflow or signed overflow occurred */
        errno = ERANGE;
        if ( flags & FL_UNSIGNED )
            number = _UI64_MAX;
        else if ( flags & FL_NEG )
            number = _I64_MIN;
        else
            number = _I64_MAX;
    }
    if (endptr != NULL)
        /* store pointer to char that stopped the scan */
        *endptr = p;

    if (flags & FL_NEG)
        /* negate result if there was a neg sign */
        number = (uint64)(-(int64)number);

    return number;            /* done. */
}

static int64 StrToInt64( AnsiString const & num_str, int ibase )
{
    return (int64)__StrToXQ( num_str.c_str(), NULL, ibase, 0 );
}

static uint64 StrToUint64( AnsiString const & num_str, int ibase )
{
    return __StrToXQ( num_str.c_str(), NULL, ibase, FL_UNSIGNED );
}

// class Buffer ----------------------------------------------------------------------
Buffer::Buffer() : _buf(NULL), _actualSize(0U), _isPeek(false)
{
}

Buffer::Buffer( void * buf, uint size, bool isPeek ) : _buf(NULL), _actualSize(0U), _isPeek(false)
{
    this->setBuf( buf, size, isPeek );
}

Buffer::~Buffer()
{
    this->free();
}

Buffer::Buffer( Buffer const & other ) : _buf(NULL), _actualSize(0U), _isPeek(false)
{
    this->setBuf( other.getBuf(), other.getSize(), other._isPeek );
}

Buffer & Buffer::operator = ( Buffer const & other )
{
    if ( this != &other )
    {
        this->setBuf( other.getBuf(), other.getSize(), other._isPeek );
    }
    return *this;
}

#ifndef MOVE_SEMANTICS_DISABLED

Buffer::Buffer( Buffer && other ) : _buf(other._buf), _actualSize(other._actualSize), _isPeek(other._isPeek)
{
    other._buf = NULL;
    other._actualSize = 0U;
    other._isPeek = false;
}

Buffer & Buffer::operator = ( Buffer && other )
{
    if ( this != &other )
    {
        this->free();

        this->_buf = other._buf;
        this->_actualSize = other._actualSize;
        this->_isPeek = other._isPeek;

        other._buf = NULL;
        other._actualSize = 0U;
        other._isPeek = false;
    }
    return *this;
}

Buffer::Buffer( GrowBuffer && other ) : _buf(other._buf), _actualSize(other._dataSize), _isPeek(other._isPeek)
{
    other._buf = NULL;
    other._actualSize = 0U;
    other._isPeek = false;

    other._dataSize = 0U;
}

Buffer & Buffer::operator = ( GrowBuffer && other )
{
    if ( this != &other )
    {
        this->free();

        this->_buf = other._buf;
        this->_actualSize = other._dataSize;
        this->_isPeek = other._isPeek;

        other._buf = NULL;
        other._actualSize = 0U;
        other._isPeek = false;

        other._dataSize = 0U;
    }
    return *this;
}

#endif

void Buffer::setBuf( void * buf, uint size, bool isPeek /*= false */ )
{
    this->free();
    this->_buf = buf;
    this->_actualSize = size;
    this->_isPeek = isPeek;
    if ( !this->_isPeek && buf != NULL ) // 如果不是窥探模式，则需要拷贝数据
    {
        this->_buf = _alloc(this->_actualSize);
        memcpy( this->_buf, buf, size );
    }
}

void Buffer::alloc( uint size )
{
    this->free();
    this->_actualSize = size;
    this->_isPeek = false;
    this->_buf = _alloc(size);
    memset( this->_buf, 0, size );
}

void Buffer::realloc( uint newSize )
{
    if ( this->_isPeek ) // 如果是窥探模式首先要变为拷贝模式
    {
        peekCopy();
    }
    this->_buf = _realloc( this->_buf, newSize );
    if ( newSize > this->_actualSize ) // 当新大小大于当前，则初始化大于的部分为0
    {
        memset( ((byte *)this->_buf) + this->_actualSize, 0, newSize - this->_actualSize );
    }
    this->_actualSize = newSize;
}

bool Buffer::peekCopy()
{
    if ( this->_isPeek )
    {
        if ( this->_buf != NULL )
        {
            void * buf = this->_buf;
            this->_buf = _alloc(this->_actualSize);
            memcpy( this->_buf, buf, this->_actualSize );
        }
        this->_isPeek = false;
        return true;
    }
    return false;
}

void Buffer::free()
{
    if ( this->_buf != NULL && !this->_isPeek )
    {
        _free(this->_buf);
        this->_buf = NULL;
        this->_actualSize = 0U;
        this->_isPeek = false;
    }
}

winux::uint Buffer::getSize() const
{
    return _actualSize;
}

void * Buffer::_alloc( uint size )
{
    return malloc(size);
}

void * Buffer::_realloc( void * p, uint newSize )
{
    return ::realloc( p, newSize );
}

void Buffer::_free( void * p )
{
    ::free(p);
}

// class GrowBuffer -----------------------------------------------------------------------
GrowBuffer::GrowBuffer( uint initSize ) : _dataSize(0)
{
    if ( initSize )
    {
        this->alloc(initSize);
    }
}

GrowBuffer::GrowBuffer( GrowBuffer const & other )
{
    this->setBuf( other._buf, other._actualSize, other._isPeek );

    this->_dataSize = other._dataSize;
}

GrowBuffer & GrowBuffer::operator = ( GrowBuffer const & other )
{
    if ( this != &other )
    {
        this->setBuf( other._buf, other._actualSize, other._isPeek );
        this->_dataSize = other._dataSize;
    }
    return *this;
}

GrowBuffer::GrowBuffer( Buffer const & other ) : Buffer(other)
{
    this->_dataSize = other._actualSize;
}

GrowBuffer & GrowBuffer::operator = ( Buffer const & other )
{
    Buffer::operator = (other);
    this->_dataSize = other._actualSize;
    return *this;
}

#ifndef MOVE_SEMANTICS_DISABLED

GrowBuffer::GrowBuffer( GrowBuffer && other ) : Buffer( std::move(other) )
{
    this->_dataSize = other._dataSize;
    other._dataSize = 0;
}

GrowBuffer & GrowBuffer::operator = ( GrowBuffer && other )
{
    if ( this != &other )
    {
        Buffer::operator = ( std::move(other) );
        this->_dataSize = other._dataSize;
        other._dataSize = 0;
    }
    return *this;
}

GrowBuffer::GrowBuffer( Buffer && other ) : Buffer( std::move(other) )
{
    this->_dataSize = this->_actualSize;
}

GrowBuffer & GrowBuffer::operator = ( Buffer && other )
{
    if ( this != &other )
    {
        Buffer::operator = ( std::move(other) );
        this->_dataSize = this->_actualSize;
    }
    return *this;
}

#endif

void GrowBuffer::free()
{
    Buffer::free();
    this->_dataSize = 0;
}

winux::uint GrowBuffer::getSize() const
{
    return _dataSize;
}

static inline uint __AutoIncrement( uint n )
{
    return n / 3 + 1;
}

void GrowBuffer::append( void * data, uint size )
{
    if ( this->_dataSize + size > this->_actualSize ) // 需要重新分配大小
    {
        this->realloc( this->_dataSize + size + __AutoIncrement( this->_dataSize + size ) );
    }
    memcpy( (winux::byte *)this->_buf + this->_dataSize, data, size );
    this->_dataSize += size;
}

// 1 2 3 4 5 6 7 8 9 10
// 0 1 2 3 4 5 6 7 8 9
void GrowBuffer::erase( uint start, uint count )
{
    if ( start >= 0 && start < _dataSize )
    {
        if ( count == (uint)-1 || count >= _dataSize - start ) // 如果count=-1或者count>=数据可删除量,就全部删除
        {
            _dataSize = start;
        }
        else
        {
            // 将后面数据覆盖掉删除的数据，并调整数据大小
            memmove( (winux::byte *)_buf + start, (winux::byte *)_buf + start + count, _dataSize - ( start + count ) );
            _dataSize -= count;
        }

        if ( _dataSize > 0 )
        {
            double delta = _actualSize / (double)_dataSize;
            if ( delta > 1.5 )
                this->realloc(_dataSize);
        }
        else
        {
            if ( _actualSize - _dataSize > 100 )
            {
                this->realloc(_dataSize);
            }
        }
    }
}

// class Mixed::MixedLess -----------------------------------------------------------------
bool Mixed::MixedLess::operator () ( Mixed const & v1, Mixed const & v2 ) const
{
    if ( v1._type == v2._type )
    {
        switch ( v1._type )
        {
        case MT_NULL:
            return false;
            break;
        case MT_BOOLEAN:
            return v1._boolVal < v2._boolVal;
            break;
        case MT_BYTE:
            return v1._btVal < v2._btVal;
            break;
        case MT_SHORT:
            return v1._shVal < v2._shVal;
            break;
        case MT_USHORT:
            return v1._ushVal < v2._ushVal;
            break;
        case MT_INT:
            return v1._iVal < v2._iVal;
            break;
        case MT_UINT:
            return v1._uiVal < v2._uiVal;
            break;
        case MT_LONG:
            return v1._lVal < v2._lVal;
            break;
        case MT_ULONG:
            return v1._ulVal < v2._ulVal;
            break;
        case MT_INT64:
            return v1._i64Val < v2._i64Val;
            break;
        case MT_UINT64:
            return v1._ui64Val < v2._ui64Val;
            break;
        case MT_FLOAT:
            return v1._fltVal < v2._fltVal;
            break;
        case MT_DOUBLE:
            return v1._dblVal < v2._dblVal;
            break;
        case MT_ANSI:
            return *v1._pStr < *v2._pStr;
            break;
        case MT_UNICODE:
            return *v1._pWStr < *v2._pWStr;
            break;
        case MT_ARRAY:
            break;
        case MT_COLLECTION:
            break;
        case MT_BINARY:
            break;
        }
    }
    else if ( v1._type > v2._type )
    {
        switch ( v1._type )
        {
        case MT_NULL:
            return false;
            break;
        case MT_BOOLEAN:
            return v1._boolVal < (bool)v2;
            break;
        case MT_BYTE:
            return v1._btVal < (byte)v2;
            break;
        case MT_SHORT:
            return v1._shVal < (short)v2;
            break;
        case MT_USHORT:
            return v1._ushVal < (ushort)v2;
            break;
        case MT_INT:
            return v1._iVal < (int)v2;
            break;
        case MT_UINT:
            return v1._uiVal < (uint)v2;
            break;
        case MT_LONG:
            return v1._lVal < (long)v2;
            break;
        case MT_ULONG:
            return v1._ulVal < (ulong)v2;
            break;
        case MT_INT64:
            return v1._i64Val < (int64)v2;
            break;
        case MT_UINT64:
            return v1._ui64Val < (uint64)v2;
            break;
        case MT_FLOAT:
            return v1._fltVal < (float)v2;
            break;
        case MT_DOUBLE:
            return v1._dblVal < (double)v2;
            break;
        case MT_ANSI:
            return *v1._pStr < (AnsiString)v2;
            break;
        case MT_UNICODE:
            return *v1._pWStr < (UnicodeString)v2;
            break;
        case MT_ARRAY:
            break;
        case MT_COLLECTION:
            break;
        case MT_BINARY:
            break;
        }
    }
    else
    {
        switch ( v2._type )
        {
        case MT_NULL:
            return false;
            break;
        case MT_BOOLEAN:
            return (bool)v1 < v2._boolVal;
            break;
        case MT_BYTE:
            return (byte)v1 < v2._btVal;
            break;
        case MT_SHORT:
            return (short)v1 < v2._shVal;
            break;
        case MT_USHORT:
            return (ushort)v1 < v2._ushVal;
            break;
        case MT_INT:
            return (int)v1 < v2._iVal;
            break;
        case MT_UINT:
            return (uint)v1 < v2._uiVal;
            break;
        case MT_LONG:
            return (long)v1 < v2._lVal;
            break;
        case MT_ULONG:
            return (ulong)v1 < v2._ulVal;
            break;
        case MT_INT64:
            return (int64)v1 < v2._i64Val;
            break;
        case MT_UINT64:
            return (uint64)v1 < v2._ui64Val;
            break;
        case MT_FLOAT:
            return (float)v1 < v2._fltVal;
            break;
        case MT_DOUBLE:
            return (double)v1 < v2._dblVal;
            break;
        case MT_ANSI:
            return (AnsiString)v1 < *v2._pStr;
            break;
        case MT_UNICODE:
            return (UnicodeString)v1 < *v2._pWStr;
            break;
        case MT_ARRAY:
            break;
        case MT_COLLECTION:
            break;
        case MT_BINARY:
            break;
        }
    }

    return ((String)v1) < ((String)v2);
}

// class Mixed::CollectionAssigner -------------------------------------------------------
Mixed::CollectionAssigner & Mixed::CollectionAssigner::operator()( Mixed const & k, Mixed const & v )
{
    if ( _mx->isCollection() )
    {
        _mx->_addUniqueKey(k);
        _mx->_pMap->operator[](k) = v;
    }
    return *this;
}

// class Mixed::ArrayAssigner -------------------------------------------------------------
Mixed::ArrayAssigner & Mixed::ArrayAssigner::operator()( Mixed const & v )
{
    if ( _mx->isArray() )
    {
        _mx->_pArr->push_back(v);
    }
    return *this;
}

// enum Mixed::MixedType strings -------------------------------------------------
static String __MixedTypeStrings[] = {
    MixedType_ENUM_ITEMLIST(MixedType_ENUM_ITEMSTRING)
};

String const & Mixed::TypeString( MixedType type )
{
    return __MixedTypeStrings[type];
}

// class Mixed ----------------------------------------------------------------------------
void Mixed::_zeroInit()
{
    memset( this, 0, sizeof(Mixed) );
    this->_type = MT_NULL;
}

/*void Mixed::_addUniqueKey( Mixed const & v )
{
    int i;
    for ( i = 0; i < (int)this->_pArr->size(); ++i )
    {
        if ( (*this->_pArr)[i] == v )
        {
            return;//i;
        }
    }

    //i = this->_pArr->size();
    this->_pArr->push_back(v);
    //return i;
}*/

Mixed::Mixed()
{
    _zeroInit();
}

Mixed::Mixed( AnsiString const & str )
{
    _zeroInit();
    this->assign( str.c_str(), str.length() );
}

Mixed::Mixed( UnicodeString const & str )
{
    _zeroInit();
    this->assign( str.c_str(), str.length() );
}

Mixed::Mixed( char const * str, int len ) // 多字节字符串
{
    _zeroInit();
    this->assign( str, len );
}

Mixed::Mixed( wchar const * str, int len ) // Unicode字符串
{
    _zeroInit();
    this->assign( str, len );
}

Mixed::Mixed( bool boolVal )
{
    _zeroInit();
    this->assign(boolVal);
}

Mixed::Mixed( byte btVal )
{
    _zeroInit();
    this->assign(btVal);
}

Mixed::Mixed( short shVal )
{
    _zeroInit();
    this->assign(shVal);
}

Mixed::Mixed( ushort ushVal )
{
    _zeroInit();
    this->assign(ushVal);
}

Mixed::Mixed( int iVal )
{
    _zeroInit();
    this->assign(iVal);
}

Mixed::Mixed( uint uiVal )
{
    _zeroInit();
    this->assign(uiVal);
}

Mixed::Mixed( long lVal )
{
    _zeroInit();
    this->assign(lVal);
}

Mixed::Mixed( ulong ulVal )
{
    _zeroInit();
    this->assign(ulVal);
}

Mixed::Mixed( float fltVal )
{
    _zeroInit();
    this->assign(fltVal);
}

Mixed::Mixed( int64 i64Val )
{
    _zeroInit();
    this->assign(i64Val);
}

Mixed::Mixed( uint64 ui64Val )
{
    _zeroInit();
    this->assign(ui64Val);
}

Mixed::Mixed( double dblVal )
{
    _zeroInit();
    this->assign(dblVal);
}

Mixed::Mixed( Buffer const & buf )
{
    _zeroInit();
    this->assign(buf);
}

Mixed::Mixed( void * binaryData, uint size, bool isPeek )
{
    _zeroInit();
    this->assign( binaryData, size, isPeek );
}

Mixed::Mixed( Mixed * arr, uint count )
{
    _zeroInit();
    this->assign( arr, count );
}

Mixed::~Mixed()
{
    this->free();
}

Mixed::Mixed( Mixed const & other )
{
    _zeroInit();
    this->operator = (other);
}

Mixed & Mixed::operator = ( Mixed const & other )
{
    if ( this == &other ) goto RETURN;
    this->free(); // 先释放清空自己
    switch ( other._type )
    {
    case MT_ANSI:
        this->_type = other._type;
        this->_pStr = new AnsiString(*other._pStr);
        break;
    case MT_UNICODE:
        this->_type = other._type;
        this->_pWStr = new UnicodeString(*other._pWStr);
        break;
    case MT_ARRAY:
        this->_type = other._type;
        this->_pArr = new MixedArray(*other._pArr);
        break;
    case MT_COLLECTION:
        this->_type = other._type;
        this->_pArr = new MixedArray(*other._pArr);
        this->_pMap = new MixedMixedMap(*other._pMap);
        break;
    case MT_BINARY:
        this->_type = other._type;
        this->_pBuf = new Buffer(*other._pBuf);
        break;
    default: // 是其他的话，直接copy就好
        memcpy( this, &other, sizeof( other ) );
        break;
    }
RETURN:
    return *this;
}

#ifndef MOVE_SEMANTICS_DISABLED

Mixed::Mixed( Mixed && other )
{
    memcpy( this, &other, sizeof(Mixed) );
    other._zeroInit();
}

Mixed & Mixed::operator = ( Mixed && other )
{
    if ( this != &other )
    {
        this->free();
        memcpy( this, &other, sizeof(Mixed) );
        other._zeroInit();
    }
    return *this;
}

Mixed::Mixed( Buffer && buf )
{
    this->_type = MT_BINARY;
    this->_pBuf = new Buffer( std::move(buf) );
}

void Mixed::assign( Buffer && buf )
{
    this->free();
    this->_type = MT_BINARY;
    this->_pBuf = new Buffer( std::move(buf) );
}

Mixed::Mixed( GrowBuffer && buf )
{
    this->_type = MT_BINARY;
    this->_pBuf = new Buffer( std::move(buf) );
}

void Mixed::assign( GrowBuffer && buf )
{
    this->free();
    this->_type = MT_BINARY;
    this->_pBuf = new Buffer( std::move(buf) );
}

#endif

void Mixed::free()
{
    switch ( this->_type )
    {
    case MT_ANSI:
        if ( this->_pStr != NULL )
        {
            delete this->_pStr;
        }
        break;
    case MT_UNICODE:
        if ( this->_pWStr != NULL )
        {
            delete this->_pWStr;
        }
        break;
    case MT_ARRAY:
        if ( this->_pArr != NULL )
        {
            delete this->_pArr;
        }
        break;
    case MT_COLLECTION:
        if ( this->_pArr != NULL )
        {
            delete this->_pArr;
        }
        if ( this->_pMap != NULL )
        {
            delete this->_pMap;
        }
        break;
    case MT_BINARY:
        if ( this->_pBuf != NULL )
        {
            delete this->_pBuf;
        }
        break;
    default:
        break;
    }
    this->_zeroInit();
}

Mixed::operator AnsiString() const
{
    // other to string
    AnsiString s;
    switch ( this->_type )
    {
    case MT_BOOLEAN:
        s = this->_boolVal ? "1" : "";
        break;
    case MT_BYTE:
        {
            char buf[10] = { 0 };
            sprintf( buf, "0x%02X", (uint)this->_btVal );
            s = buf;
        }
        break;
    case MT_SHORT:
        {
            std::ostringstream sout;
            sout << this->_shVal;
            s = sout.str();
        }
        break;
    case MT_USHORT:
        {
            std::ostringstream sout;
            sout << this->_ushVal;
            s = sout.str();
        }
        break;
    case MT_INT:
        {
            std::ostringstream sout;
            sout << this->_iVal;
            s = sout.str();
        }
        break;
    case MT_UINT:
        {
            std::ostringstream sout;
            sout << this->_uiVal;
            s = sout.str();
        }
        break;
    case MT_LONG:
        {
            std::ostringstream sout;
            sout << this->_lVal;
            s = sout.str();
        }
        break;
    case MT_ULONG:
        {
            std::ostringstream sout;
            sout << this->_ulVal;
            s = sout.str();
        }
        break;
    case MT_INT64:
        {
            char buf[30] = { 0 };
        #if defined(_MSC_VER) || defined(WIN32)
            sprintf( buf, "%I64d", this->_i64Val );
        #else
            sprintf( buf, "%lld", this->_i64Val );
        #endif // IS_WINDOWS
            s = buf;
        }
        break;
    case MT_UINT64:
        {
            char buf[30] = { 0 };
        #if defined(_MSC_VER) || defined(WIN32)
            sprintf( buf, "%I64u", this->_ui64Val );
        #else
            sprintf( buf, "%llu", this->_ui64Val );
        #endif
            s = buf;
        }
        break;
    case MT_FLOAT:
        {
            std::ostringstream sout;
            sout << std::setprecision(6) << this->_fltVal;
            s = sout.str();
        }
        break;
    case MT_DOUBLE:
        {
            std::ostringstream sout;
            sout << std::setprecision(15) << this->_dblVal;
            s = sout.str();
        }
        break;
    case MT_ANSI:
        {
            s = *this->_pStr;
        }
        break;
    case MT_UNICODE:
        {
            s = UnicodeToLocal(*this->_pWStr);
        }
        break;
    case MT_ARRAY:
        s = MixedToJsonA( *this, false );
        break;
    case MT_COLLECTION:
        s = MixedToJsonA( *this, false );
        break;
    case MT_BINARY:
        return BufferToAnsiString( this->_pBuf->getBuf(), this->_pBuf->getSize() );
        break;
    default:
        //s = "null";
        break;
    }
    return s;
}

Mixed::operator UnicodeString() const
{
    // other to unicode
    UnicodeString s;
    switch ( this->_type )
    {
    case MT_BOOLEAN:
        s = this->_boolVal ? L"1" : L"";
        break;
    case MT_BYTE:
        {
            wchar buf[10] = { 0 };
        #if defined(_MSC_VER) || defined(WIN32)
            swprintf( buf, L"0x%02X", this->_btVal );
        #else
            swprintf( buf, 4, L"0x%02X", this->_btVal );
        #endif
            s = buf;
        }
        break;
    case MT_SHORT:
        {
            std::wostringstream sout;
            sout << this->_shVal;
            s = sout.str();
        }
        break;
    case MT_USHORT:
        {
            std::wostringstream sout;
            sout << this->_ushVal;
            s = sout.str();
        }
        break;
    case MT_INT:
        {
            std::wostringstream sout;
            sout << this->_iVal;
            s = sout.str();
        }
        break;
    case MT_UINT:
        {
            std::wostringstream sout;
            sout << this->_uiVal;
            s = sout.str();
        }
        break;
    case MT_LONG:
        {
            std::wostringstream sout;
            sout << this->_lVal;
            s = sout.str();
        }
        break;
    case MT_ULONG:
        {
            std::wostringstream sout;
            sout << this->_ulVal;
            s = sout.str();
        }
        break;
    case MT_INT64:
        {
            wchar buf[30] = { 0 };
        #if defined(_MSC_VER) || defined(WIN32)
            swprintf( buf, L"%I64d", this->_i64Val );
        #else
            swprintf( buf, 30, L"%lld", this->_i64Val );
        #endif
            s = buf;

        }
        break;
    case MT_UINT64:
        {
            wchar buf[30] = { 0 };
        #if defined(_MSC_VER) || defined(WIN32)
            swprintf( buf, L"%I64u", this->_ui64Val );
        #else
            swprintf( buf, 30, L"%llu", this->_ui64Val );
        #endif
            s = buf;
        }
        break;
    case MT_FLOAT:
        {
            std::wostringstream sout;
            sout << std::setprecision(6) << this->_fltVal;
            s = sout.str();
        }
        break;
    case MT_DOUBLE:
        {
            std::wostringstream sout;
            sout << std::setprecision(15) << this->_dblVal;
            s = sout.str();
        }
        break;
    case MT_ANSI:
        {
            s = LocalToUnicode(*this->_pStr);
        }
        break;
    case MT_UNICODE:
        {
            s = *this->_pWStr;
        }
        break;
    case MT_ARRAY:
        s = MixedToJsonW( *this, false );
        break;
    case MT_COLLECTION:
        s = MixedToJsonW( *this, false );
        break;
    case MT_BINARY:
        s.resize( this->getSize() / sizeof(wchar) + 1 );
        memcpy( &s[0], this->getBuf(), this->getSize() );
        break;
    default:
        //s = L"null";
        break;
    }
    return s;
}

Mixed::operator Buffer() const
{
    Buffer buf;
    switch ( this->_type )
    {
    case MT_BOOLEAN:
        {
            buf.alloc( sizeof(this->_boolVal) );
            memcpy( buf.getBuf(), &this->_boolVal, buf.getSize() );
        }
        break;
    case MT_BYTE:
        {
            buf.alloc( sizeof(this->_btVal) );
            memcpy( buf.getBuf(), &this->_btVal, buf.getSize() );
        }
        break;
    case MT_SHORT:
        {
            buf.alloc( sizeof(this->_shVal) );
            memcpy( buf.getBuf(), &this->_shVal, buf.getSize() );
        }
        break;
    case MT_USHORT:
        {
            buf.alloc( sizeof(this->_ushVal) );
            memcpy( buf.getBuf(), &this->_ushVal, buf.getSize() );
        }
        break;
    case MT_INT:
        {
            buf.alloc( sizeof(this->_iVal) );
            memcpy( buf.getBuf(), &this->_iVal, buf.getSize() );
        }
        break;
    case MT_UINT:
        {
            buf.alloc( sizeof(this->_uiVal) );
            memcpy( buf.getBuf(), &this->_uiVal, buf.getSize() );
        }
        break;
    case MT_LONG:
        {
            buf.alloc( sizeof(this->_lVal) );
            memcpy( buf.getBuf(), &this->_lVal, buf.getSize() );
        }
        break;
    case MT_ULONG:
        {
            buf.alloc( sizeof(this->_ulVal) );
            memcpy( buf.getBuf(), &this->_ulVal, buf.getSize() );
        }
        break;
    case MT_INT64:
        {
            buf.alloc( sizeof(this->_i64Val) );
            memcpy( buf.getBuf(), &this->_i64Val, buf.getSize() );
        }
        break;
    case MT_UINT64:
        {
            buf.alloc( sizeof(this->_ui64Val) );
            memcpy( buf.getBuf(), &this->_ui64Val, buf.getSize() );
        }
        break;
    case MT_FLOAT:
        {
            buf.alloc( sizeof(this->_fltVal) );
            memcpy( buf.getBuf(), &this->_fltVal, buf.getSize() );
        }
        break;
    case MT_DOUBLE:
        {
            buf.alloc( sizeof(this->_dblVal) );
            memcpy( buf.getBuf(), &this->_dblVal, buf.getSize() );
        }
        break;
    case MT_ANSI:
        {
            buf.alloc( this->_pStr->size() );
            memcpy( buf.getBuf(), this->_pStr->c_str(), buf.getSize() );
        }
        break;
    case MT_UNICODE:
        {
            buf.alloc( this->_pWStr->size() * sizeof(UnicodeString::value_type) );
            memcpy( buf.getBuf(), this->_pWStr->c_str(), buf.getSize() );
        }
        break;
    case MT_ARRAY:
        {
            //s = MixedToJsonW( *this, false );
            GrowBuffer tmpBuf(64);
            int n = (int)this->_pArr->size();
            for ( int i = 0; i < n; ++i )
            {
                Buffer aBuf = this->_pArr->at(i).operator Buffer();
                tmpBuf.append( aBuf.getBuf(), aBuf.getSize() );
            }
            buf = std::move(tmpBuf);
        }
        break;
    case MT_COLLECTION:
        {
            //s = MixedToJsonW( *this, false );
            GrowBuffer tmpBuf(64);
            int n = this->getCount();
            for ( int i = 0; i < n; ++i )
            {
                Buffer aBuf = this->getPair(i).second.operator Buffer();
                tmpBuf.append( aBuf.getBuf(), aBuf.getSize() );
            }
            buf = std::move(tmpBuf);
        }
        break;
    case MT_BINARY:
        {
            buf.setBuf( this->_pBuf->getBuf(), this->_pBuf->getSize(), false );
        }
        break;
    default:
        // empty buffer.
        break;
    }
    return buf;
}

Mixed::operator bool() const
{
    bool b = false;
    switch ( this->_type )
    {
    case MT_BOOLEAN:
        b = this->_boolVal;
        break;
    case MT_BYTE:
        b = this->_btVal != 0;
        break;
    case MT_SHORT:
        b = this->_shVal != 0;
        break;
    case MT_USHORT:
        b = this->_ushVal != 0;
        break;
    case MT_INT:
        b = this->_iVal != 0;
        break;
    case MT_UINT:
        b = this->_uiVal != 0;
        break;
    case MT_LONG:
        b = this->_lVal != 0;
        break;
    case MT_ULONG:
        b = this->_ulVal != 0;
        break;
    case MT_INT64:
        b = this->_i64Val != 0;
        break;
    case MT_UINT64:
        b = this->_ui64Val != 0;
        break;
    case MT_FLOAT:
        b = this->_fltVal != 0;
        break;
    case MT_DOUBLE:
        b = this->_dblVal != 0;
        break;
    case MT_ANSI: // 字符串转成bool型,空串为false,否则为true
        b = !this->_pStr->empty();
        break;
    case MT_UNICODE:
        b = !this->_pWStr->empty();
        break;
    case MT_ARRAY:
        return !this->_pArr->empty();
        break;
    case MT_COLLECTION:
        return !this->_pMap->empty();
        break;
    case MT_BINARY:
        return this->_pBuf->getSize() > 0;
        break;
    default:
        break;
    }
    return b;
}

Mixed::operator byte() const
{
    byte bt = 0U;
    switch ( this->_type )
    {
    case MT_BOOLEAN:
        bt = (byte)this->_boolVal;
        break;
    case MT_BYTE:
        bt = this->_btVal;
        break;
    case MT_SHORT:
        bt = (byte)this->_shVal;
        break;
    case MT_USHORT:
        bt = (byte)this->_ushVal;
        break;
    case MT_INT:
        bt = (byte)this->_iVal;
        break;
    case MT_UINT:
        bt = (byte)this->_uiVal;
        break;
    case MT_LONG:
        bt = (byte)this->_lVal;
        break;
    case MT_ULONG:
        bt = (byte)this->_ulVal;
        break;
    case MT_INT64:
        bt = (byte)this->_i64Val;
        break;
    case MT_UINT64:
        bt = (byte)this->_ui64Val;
        break;
    case MT_FLOAT:
        bt = (byte)this->_fltVal;
        break;
    case MT_DOUBLE:
        bt = (byte)this->_dblVal;
        break;
    case MT_ANSI:
        bt = (byte)this->operator ulong();
        break;
    case MT_UNICODE:
        bt = (byte)this->operator ulong();
        break;
    case MT_ARRAY:
        throw MixedError( MixedError::meCantConverted, "MT_ARRAY can't convert to MT_BYTE" );
        break;
    case MT_COLLECTION:
        throw MixedError( MixedError::meCantConverted, "MT_COLLECTION can't convert to MT_BYTE" );
        break;
    case MT_BINARY:
        throw MixedError( MixedError::meCantConverted, "MT_BINARY can't convert to MT_BYTE" );
        break;
    default:
        break;
    }
    return bt;
}

Mixed::operator short() const
{
    switch ( this->_type )
    {
    case MT_SHORT:
        return this->_shVal;
    case MT_ARRAY:
        throw MixedError( MixedError::meCantConverted, "MT_ARRAY can't convert to MT_SHORT" );
        break;
    case MT_COLLECTION:
        throw MixedError( MixedError::meCantConverted, "MT_COLLECTION can't convert to MT_SHORT" );
        break;
    case MT_BINARY:
        throw MixedError( MixedError::meCantConverted, "MT_BINARY can't convert to MT_SHORT" );
        break;
    default:
        return (short)this->operator long();
    }
}

Mixed::operator ushort() const
{
    switch ( this->_type )
    {
    case MT_USHORT:
        return this->_ushVal;
    case MT_ARRAY:
        throw MixedError( MixedError::meCantConverted, "MT_ARRAY can't convert to MT_USHORT" );
        break;
    case MT_COLLECTION:
        throw MixedError( MixedError::meCantConverted, "MT_COLLECTION can't convert to MT_USHORT" );
        break;
    case MT_BINARY:
        throw MixedError( MixedError::meCantConverted, "MT_BINARY can't convert to MT_USHORT" );
        break;
    default:
        return (ushort)this->operator ulong();
    }
}

Mixed::operator int() const
{
    switch ( this->_type )
    {
    case MT_INT:
        return this->_iVal;
    case MT_ARRAY:
        throw MixedError( MixedError::meCantConverted, "MT_ARRAY can't convert to MT_INT" );
        break;
    case MT_COLLECTION:
        throw MixedError( MixedError::meCantConverted, "MT_COLLECTION can't convert to MT_INT" );
        break;
    case MT_BINARY:
        throw MixedError( MixedError::meCantConverted, "MT_BINARY can't convert to MT_INT" );
        break;
    default:
        return (int)this->operator long();
    }
}

Mixed::operator uint() const
{
    switch ( this->_type )
    {
    case MT_UINT:
        return this->_uiVal;
    case MT_ARRAY:
        throw MixedError( MixedError::meCantConverted, "MT_ARRAY can't convert to MT_UINT" );
        break;
    case MT_COLLECTION:
        throw MixedError( MixedError::meCantConverted, "MT_COLLECTION can't convert to MT_UINT" );
        break;
    case MT_BINARY:
        throw MixedError( MixedError::meCantConverted, "MT_BINARY can't convert to MT_UINT" );
        break;
    default:
        return (uint)this->operator ulong();
    }
}

Mixed::operator long() const
{
    switch ( this->_type )
    {
    case MT_LONG:
        return this->_lVal;
    case MT_ARRAY:
        throw MixedError( MixedError::meCantConverted, "MT_ARRAY can't convert to MT_LONG" );
        break;
    case MT_COLLECTION:
        throw MixedError( MixedError::meCantConverted, "MT_COLLECTION can't convert to MT_LONG" );
        break;
    case MT_BINARY:
        throw MixedError( MixedError::meCantConverted, "MT_BINARY can't convert to MT_LONG" );
        break;
    default:
        break;
    }
    union
    {
        long l;
        ulong ul;
    } tmp;
    tmp.ul = this->operator ulong();
    return tmp.l;
}

Mixed::operator ulong() const
{
    ulong ul = 0UL;
    switch ( this->_type )
    {
    case MT_BOOLEAN:
        ul = this->_boolVal ? 1U : 0U;
        break;
    case MT_BYTE:
        ul = (ulong)this->_btVal;
        break;
    case MT_SHORT:
        ul = (ulong)this->_shVal;
        break;
    case MT_USHORT:
        ul = (ulong)this->_ushVal;
        break;
    case MT_INT:
        ul = (ulong)this->_iVal;
        break;
    case MT_UINT:
        ul = (ulong)this->_uiVal;
        break;
    case MT_LONG:
        {
            union
            {
                long l;
                ulong ul;
            } tmp;
            tmp.l = this->_lVal;
            ul = tmp.ul;
        }
        break;
    case MT_ULONG:
        ul = this->_ulVal;
        break;
    case MT_INT64:
        ul = (ulong)this->_i64Val;
        break;
    case MT_UINT64:
        ul = (ulong)this->_ui64Val;
        break;
    case MT_FLOAT:
        ul = (ulong)this->_fltVal;
        break;
    case MT_DOUBLE:
        ul = (ulong)this->_dblVal;
        break;
    case MT_ANSI:
        ParseULong( *this->_pStr, &ul );
        break;
    case MT_UNICODE:
        ParseULong( *this->_pWStr, &ul );
        break;
    case MT_ARRAY:
        throw MixedError( MixedError::meCantConverted, "MT_ARRAY can't convert to MT_ULONG" );
        break;
    case MT_COLLECTION:
        throw MixedError( MixedError::meCantConverted, "MT_COLLECTION can't convert to MT_ULONG" );
        break;
    case MT_BINARY:
        throw MixedError( MixedError::meCantConverted, "MT_BINARY can't convert to MT_ULONG" );
        break;
    default:
        break;
    }
    return ul;
}

Mixed::operator float() const
{
    switch ( this->_type )
    {
    case MT_FLOAT:
        return this->_fltVal;
    case MT_ARRAY:
        throw MixedError( MixedError::meCantConverted, "MT_ARRAY can't convert to MT_FLOAT" );
        break;
    case MT_COLLECTION:
        throw MixedError( MixedError::meCantConverted, "MT_COLLECTION can't convert to MT_FLOAT" );
        break;
    case MT_BINARY:
        throw MixedError( MixedError::meCantConverted, "MT_BINARY can't convert to MT_FLOAT" );
        break;
    default:
        return (float)this->operator double();
    }
}

Mixed::operator int64() const
{
    switch ( this->_type )
    {
    case MT_INT64:
        return this->_i64Val;
    case MT_ARRAY:
        throw MixedError( MixedError::meCantConverted, "MT_ARRAY can't convert to MT_INT64" );
        break;
    case MT_COLLECTION:
        throw MixedError( MixedError::meCantConverted, "MT_COLLECTION can't convert to MT_INT64" );
        break;
    case MT_BINARY:
        throw MixedError( MixedError::meCantConverted, "MT_BINARY can't convert to MT_INT64" );
        break;
    default:
        break;
    }
    union
    {
        int64 i64;
        uint64 ui64;
    } tmp;
    tmp.ui64 = this->operator uint64();
    return tmp.i64;
}

Mixed::operator uint64() const
{
    uint64 ui64 = 0U;
    switch ( this->_type )
    {
    case MT_BOOLEAN:
        ui64 = this->_boolVal ? 1U : 0U;
        break;
    case MT_BYTE:
        ui64 = (uint64)this->_btVal;
        break;
    case MT_SHORT:
        ui64 = (uint64)this->_shVal;
        break;
    case MT_USHORT:
        ui64 = (uint64)this->_ushVal;
        break;
    case MT_INT:
        ui64 = (uint64)this->_iVal;
        break;
    case MT_UINT:
        ui64 = (uint64)this->_uiVal;
        break;
    case MT_LONG:
        ui64 = (uint64)this->_lVal;
        break;
    case MT_ULONG:
        ui64 = (uint64)this->_ulVal;
        break;
    case MT_INT64:
        ui64 = (uint64)this->_i64Val;
        break;
    case MT_UINT64:
        ui64 = this->_ui64Val;
        break;
    case MT_FLOAT:
        ui64 = (uint64)this->_fltVal;
        break;
    case MT_DOUBLE:
        ui64 = (uint64)this->_dblVal;
        break;
    case MT_ANSI:
        ParseUInt64( *this->_pStr, &ui64 );
        break;
    case MT_UNICODE:
        ParseUInt64( *this->_pWStr, &ui64 );
        break;
    case MT_ARRAY:
        throw MixedError( MixedError::meCantConverted, "MT_ARRAY can't convert to MT_UINT64" );
        break;
    case MT_COLLECTION:
        throw MixedError( MixedError::meCantConverted, "MT_COLLECTION can't convert to MT_UINT64" );
        break;
    case MT_BINARY:
        throw MixedError( MixedError::meCantConverted, "MT_BINARY can't convert to MT_UINT64" );
        break;
    default:
        break;
    }
    return ui64;
}

Mixed::operator double() const
{
    double dbl = 0.0;
    switch ( this->_type )
    {
    case MT_BOOLEAN:
        dbl = this->_boolVal ? 1.0 : 0.0;
        break;
    case MT_BYTE:
        dbl = (double)this->_btVal;
        break;
    case MT_SHORT:
        dbl = (double)this->_shVal;
        break;
    case MT_USHORT:
        dbl = (double)this->_ushVal;
        break;
    case MT_INT:
        dbl = (double)this->_iVal;
        break;
    case MT_UINT:
        dbl = (double)this->_uiVal;
        break;
    case MT_LONG:
        dbl = (double)this->_lVal;
        break;
    case MT_ULONG:
        dbl = (double)this->_ulVal;
        break;
    case MT_INT64:
        dbl = (double)this->_i64Val;
        break;
    case MT_UINT64:
        dbl = (double)(int64)this->_ui64Val;
        break;
    case MT_FLOAT:
        dbl = (double)this->_fltVal;
        break;
    case MT_DOUBLE:
        dbl = this->_dblVal;
        break;
    case MT_ANSI:
        ParseDouble( *this->_pStr, &dbl );
        break;
    case MT_UNICODE:
        ParseDouble( *this->_pWStr, &dbl );
        break;
    case MT_ARRAY:
        throw MixedError( MixedError::meCantConverted, "MT_ARRAY can't convert to MT_DOUBLE" );
        break;
    case MT_COLLECTION:
        throw MixedError( MixedError::meCantConverted, "MT_COLLECTION can't convert to MT_DOUBLE" );
        break;
    case MT_BINARY:
        throw MixedError( MixedError::meCantConverted, "MT_BINARY can't convert to MT_DOUBLE" );
        break;
    default:
        break;
    }
    return dbl;
}

// 比较操作符 ------------------------------------------------------------------------------------------------------
bool Mixed::operator == ( Mixed const & other ) const
{
    if ( this->_type == other._type )
    {
        switch ( this->_type )
        {
        case MT_NULL:
            return true;
            break;
        case MT_BOOLEAN:
            return this->_boolVal == other._boolVal;
            break;
        case MT_BYTE:
            return this->_btVal == other._btVal;
            break;
        case MT_SHORT:
            return this->_shVal == other._shVal;
            break;
        case MT_USHORT:
            return this->_ushVal == other._ushVal;
            break;
        case MT_INT:
            return this->_iVal == other._iVal;
            break;
        case MT_UINT:
            return this->_uiVal == other._uiVal;
            break;
        case MT_LONG:
            return this->_lVal == other._lVal;
            break;
        case MT_ULONG:
            return this->_ulVal == other._ulVal;
            break;
        case MT_INT64:
            return this->_i64Val == other._i64Val;
            break;
        case MT_UINT64:
            return this->_ui64Val == other._ui64Val;
            break;
        case MT_FLOAT:
            return this->_fltVal == other._fltVal;
            break;
        case MT_DOUBLE:
            return this->_dblVal == other._dblVal;
            break;
        case MT_ANSI:
            return *this->_pStr == *other._pStr;
            break;
        case MT_UNICODE:
            return *this->_pWStr == *other._pWStr;
            break;
        case MT_ARRAY:
            throw MixedError( MixedError::meUnexpectedType, "MT_ARRAY type can't support `operator == ()`" );
            break;
        case MT_COLLECTION:
            throw MixedError( MixedError::meUnexpectedType, "MT_COLLECTION type can't support `operator == ()`" );
            break;
        case MT_BINARY:
            throw MixedError( MixedError::meUnexpectedType, "MT_BINARY type can't support `operator == ()`" );
            break;
        default:
            return false;
            break;
        }
    }
    else if ( this->_type > other._type )
    {
        switch ( this->_type )
        {
        case MT_NULL:
            return other._type == MT_NULL;
            break;
        case MT_BOOLEAN:
            return this->_boolVal == (bool)other;
            break;
        case MT_BYTE:
            return this->_btVal == (byte)other;
            break;
        case MT_SHORT:
            return this->_shVal == (short)other;
            break;
        case MT_USHORT:
            return this->_ushVal == (ushort)other;
            break;
        case MT_INT:
            return this->_iVal == (int)other;
            break;
        case MT_UINT:
            return this->_uiVal == (uint)other;
            break;
        case MT_LONG:
            return this->_lVal == (long)other;
            break;
        case MT_ULONG:
            return this->_ulVal == (ulong)other;
            break;
        case MT_INT64:
            return this->_i64Val == (int64)other;
            break;
        case MT_UINT64:
            return this->_ui64Val == (uint64)other;
            break;
        case MT_FLOAT:
            return this->_fltVal == (float)other;
            break;
        case MT_DOUBLE:
            return this->_dblVal == (double)other;
            break;
        case MT_ANSI:
            return *this->_pStr == (AnsiString)other;
            break;
        case MT_UNICODE:
            return *this->_pWStr == (UnicodeString)other;
            break;
        case MT_ARRAY:
            throw MixedError( MixedError::meUnexpectedType, "MT_ARRAY type can't support `operator == ()`" );
            break;
        case MT_COLLECTION:
            throw MixedError( MixedError::meUnexpectedType, "MT_COLLECTION type can't support `operator == ()`" );
            break;
        case MT_BINARY:
            throw MixedError( MixedError::meUnexpectedType, "MT_BINARY type can't support `operator == ()`" );
            break;
        default:
            return false;
            break;
        }
    }
    else // this->_type < other._type
    {
        switch ( other._type )
        {
        case MT_NULL:
            return this->_type == MT_NULL;
            break;
        case MT_BOOLEAN:
            return (bool)*this == other._boolVal;
            break;
        case MT_BYTE:
            return (byte)*this == other._btVal;
            break;
        case MT_SHORT:
            return (short)*this == other._shVal;
            break;
        case MT_USHORT:
            return (ushort)*this == other._ushVal;
            break;
        case MT_INT:
            return (int)*this == other._iVal;
            break;
        case MT_UINT:
            return (uint)*this == other._uiVal;
            break;
        case MT_LONG:
            return (long)*this == other._lVal;
            break;
        case MT_ULONG:
            return (ulong)*this == other._ulVal;
            break;
        case MT_INT64:
            return (int64)*this == other._i64Val;
            break;
        case MT_UINT64:
            return (uint64)*this == other._ui64Val;
            break;
        case MT_FLOAT:
            return (float)*this == other._fltVal;
            break;
        case MT_DOUBLE:
            return (double)*this == other._dblVal;
            break;
        case MT_ANSI:
            return (AnsiString)*this == *other._pStr;
            break;
        case MT_UNICODE:
            return (UnicodeString)*this == *other._pWStr;
            break;
        case MT_ARRAY:
            throw MixedError( MixedError::meUnexpectedType, "MT_ARRAY type can't support `operator == ()`" );
            break;
        case MT_COLLECTION:
            throw MixedError( MixedError::meUnexpectedType, "MT_COLLECTION type can't support `operator == ()`" );
            break;
        case MT_BINARY:
            throw MixedError( MixedError::meUnexpectedType, "MT_BINARY type can't support `operator == ()`" );
            break;
        default:
            return false;
            break;
        }
    }
}

bool Mixed::operator < ( Mixed const & other ) const
{
    if ( this->_type == other._type )
    {
        switch ( this->_type )
        {
        case MT_NULL:
            return false;
            break;
        case MT_BOOLEAN:
            return this->_boolVal < other._boolVal;
            break;
        case MT_BYTE:
            return this->_btVal < other._btVal;
            break;
        case MT_SHORT:
            return this->_shVal < other._shVal;
            break;
        case MT_USHORT:
            return this->_ushVal < other._ushVal;
            break;
        case MT_INT:
            return this->_iVal < other._iVal;
            break;
        case MT_UINT:
            return this->_uiVal < other._uiVal;
            break;
        case MT_LONG:
            return this->_lVal < other._lVal;
            break;
        case MT_ULONG:
            return this->_ulVal < other._ulVal;
            break;
        case MT_INT64:
            return this->_i64Val < other._i64Val;
            break;
        case MT_UINT64:
            return this->_ui64Val < other._ui64Val;
            break;
        case MT_FLOAT:
            return this->_fltVal < other._fltVal;
            break;
        case MT_DOUBLE:
            return this->_dblVal < other._dblVal;
            break;
        case MT_ANSI:
            return *this->_pStr < *other._pStr;
            break;
        case MT_UNICODE:
            return *this->_pWStr < *other._pWStr;
            break;
        case MT_ARRAY:
            throw MixedError( MixedError::meUnexpectedType, "MT_ARRAY type can't support `operator < ()`" );
            break;
        case MT_COLLECTION:
            throw MixedError( MixedError::meUnexpectedType, "MT_COLLECTION type can't support `operator < ()`" );
            break;
        case MT_BINARY:
            throw MixedError( MixedError::meUnexpectedType, "MT_BINARY type can't support `operator < ()`" );
            break;
        default:
            return false;
            break;
        }
    }
    else if ( this->_type > other._type )
    {
        switch ( this->_type )
        {
        case MT_NULL:
            return false;
            break;
        case MT_BOOLEAN:
            return this->_boolVal < (bool)other;
            break;
        case MT_BYTE:
            return this->_btVal < (byte)other;
            break;
        case MT_SHORT:
            return this->_shVal < (short)other;
            break;
        case MT_USHORT:
            return this->_ushVal < (ushort)other;
            break;
        case MT_INT:
            return this->_iVal < (int)other;
            break;
        case MT_UINT:
            return this->_uiVal < (uint)other;
            break;
        case MT_LONG:
            return this->_lVal < (long)other;
            break;
        case MT_ULONG:
            return this->_ulVal < (ulong)other;
            break;
        case MT_INT64:
            return this->_i64Val < (int64)other;
            break;
        case MT_UINT64:
            return this->_ui64Val < (uint64)other;
            break;
        case MT_FLOAT:
            return this->_fltVal < (float)other;
            break;
        case MT_DOUBLE:
            return this->_dblVal < (double)other;
            break;
        case MT_ANSI:
            return *this->_pStr < (AnsiString)other;
            break;
        case MT_UNICODE:
            return *this->_pWStr < (UnicodeString)other;
            break;
        case MT_ARRAY:
            throw MixedError( MixedError::meUnexpectedType, "MT_ARRAY type can't support `operator < ()`" );
            break;
        case MT_COLLECTION:
            throw MixedError( MixedError::meUnexpectedType, "MT_COLLECTION type can't support `operator < ()`" );
            break;
        case MT_BINARY:
            throw MixedError( MixedError::meUnexpectedType, "MT_BINARY type can't support `operator < ()`" );
            break;
        default:
            return false;
            break;
        }
    }
    else // this->_type < other._type
    {
        switch ( other._type )
        {
        case MT_NULL:
            return false;
            break;
        case MT_BOOLEAN:
            return (bool)*this < other._boolVal;
            break;
        case MT_BYTE:
            return (byte)*this < other._btVal;
            break;
        case MT_SHORT:
            return (short)*this < other._shVal;
            break;
        case MT_USHORT:
            return (ushort)*this < other._ushVal;
            break;
        case MT_INT:
            return (int)*this < other._iVal;
            break;
        case MT_UINT:
            return (uint)*this < other._uiVal;
            break;
        case MT_LONG:
            return (long)*this < other._lVal;
            break;
        case MT_ULONG:
            return (ulong)*this < other._ulVal;
            break;
        case MT_INT64:
            return (int64)*this < other._i64Val;
            break;
        case MT_UINT64:
            return (uint64)*this < other._ui64Val;
            break;
        case MT_FLOAT:
            return (float)*this < other._fltVal;
            break;
        case MT_DOUBLE:
            return (double)*this < other._dblVal;
            break;
        case MT_ANSI:
            return (AnsiString)*this < *other._pStr;
            break;
        case MT_UNICODE:
            return (UnicodeString)*this < *other._pWStr;
            break;
        case MT_ARRAY:
            throw MixedError( MixedError::meUnexpectedType, "MT_ARRAY type can't support `operator < ()`" );
            break;
        case MT_COLLECTION:
            throw MixedError( MixedError::meUnexpectedType, "MT_COLLECTION type can't support `operator < ()`" );
            break;
        case MT_BINARY:
            throw MixedError( MixedError::meUnexpectedType, "MT_BINARY type can't support `operator < ()`" );
            break;
        default:
            return false;
            break;
        }
    }
}

// Assign -----------------------------------------------------------------------------------------------------------------
void Mixed::assign( char const * str, int len )
{
    this->free();
    str = str ? str : "";
    this->_type = MT_ANSI; // set _type as AnsiString
    if ( len < 0 )
    {
        this->_pStr = new AnsiString(str);
    }
    else if ( len > 0 )
    {
        this->_pStr = new AnsiString( str, len );
    }
    else
    {
        this->_pStr = new AnsiString();
    }
}

void Mixed::assign( wchar const * str, int len )
{
    this->free();
    str = str ? str : L"";
    this->_type = MT_UNICODE; // set _type as UnicodeString
    if ( len < 0 )
    {
        this->_pWStr = new UnicodeString(str);
    }
    else if ( len > 0 )
    {
        this->_pWStr = new UnicodeString( str, len );
    }
    else
    {
        this->_pWStr = new UnicodeString();
    }

}

void Mixed::assign( bool boolVal )
{
    this->free();
    this->_type = MT_BOOLEAN;
    this->_boolVal = boolVal;
}

void Mixed::assign( byte btVal )
{
    this->free();
    this->_type = MT_BYTE;
    this->_btVal = btVal;
}

void Mixed::assign( short shVal )
{
    this->free();
    this->_type = MT_SHORT;
    this->_shVal = shVal;
}

void Mixed::assign( ushort ushVal )
{
    this->free();
    this->_type = MT_USHORT;
    this->_ushVal = ushVal;
}

void Mixed::assign( int iVal )
{
    this->free();
    this->_type = MT_INT;
    this->_iVal = iVal;
}

void Mixed::assign( uint uiVal )
{
    this->free();
    this->_type = MT_UINT;
    this->_uiVal = uiVal;
}

void Mixed::assign( long lVal )
{
    this->free();
    this->_type = MT_LONG;
    this->_lVal = lVal;
}

void Mixed::assign( ulong ulVal )
{
    this->free();
    this->_type = MT_ULONG;
    this->_ulVal = ulVal;
}

void Mixed::assign( float fltVal )
{
    this->free();
    this->_type = MT_FLOAT;
    this->_fltVal = fltVal;
}

void Mixed::assign( int64 i64Val )
{
    this->free();
    this->_type = MT_INT64;
    this->_i64Val = i64Val;
}

void Mixed::assign( uint64 ui64Val )
{
    this->free();
    this->_type = MT_UINT64;
    this->_ui64Val = ui64Val;
}

void Mixed::assign( double dblVal )
{
    this->free();
    this->_type = MT_DOUBLE;
    this->_dblVal = dblVal;
}

void Mixed::assign( Buffer const & buf )
{
    this->free();
    this->_type = MT_BINARY;
    this->_pBuf = new Buffer(buf);
}

void Mixed::assign( void * binaryData, uint size, bool isPeek )
{
    this->free();
    this->_type = MT_BINARY;
    this->_pBuf = new Buffer( binaryData, size, isPeek );
}

void Mixed::assign( Mixed * arr, uint count )
{
    this->free();
    this->_type = MT_ARRAY;
    this->_pArr = new MixedArray( arr, arr + count );
}

Mixed & Mixed::createString()
{
    this->assign( "", 0 );
    return *this;
}

Mixed & Mixed::createUnicode()
{
    this->assign( L"", 0 );
    return *this;
}

Mixed & Mixed::createArray( uint count /*= 0 */ )
{
    this->free();
    this->_type = MT_ARRAY;
    if ( count > 0 )
    {
        this->_pArr = new MixedArray(count);
    }
    else
    {
        this->_pArr = new MixedArray();
    }
    return  *this;
}

Mixed & Mixed::createCollection()
{
    this->free();
    this->_type = MT_COLLECTION;
    this->_pArr = new MixedArray();
    this->_pMap = new MixedMixedMap();
    return *this;
}

Mixed & Mixed::createBuffer( uint size )
{
    this->free();
    this->_type = MT_BINARY;
    this->_pBuf = new Buffer();
    this->_pBuf->alloc(size);
    return *this;
}

// array/collection有关的操作 --------------------------------------------------------------
int Mixed::getCount() const
{
    if ( ( this->isArray() || this->isCollection() ) && this->_pArr != NULL )
    {
        return this->_pArr->size();
    }
    return 0;
}

Mixed & Mixed::operator [] ( Mixed const & k )
{
    if ( this->isArray() )
    {
        int i = k;
        if ( i < 0 || i >= (int)this->_pArr->size() )
            throw MixedError( MixedError::meOutOfArrayRange, Format( "数组越界: index:%d, size:%d", i, (int)this->_pArr->size() ).c_str() );

        return this->_pArr->operator [] (k);
    }
    else if ( this->isCollection() )
    {
        this->_addUniqueKey(k);
        //if ( this->_pMap->find(k) == this->_pMap->end() )
        //    this->_pArr->push_back(k);
        return this->_pMap->operator [] (k);
    }
    else
    {
        throw MixedError( MixedError::meUnexpectedType, this->typeString() + " can't support " + __FUNCTION__ + "(" + k.toAnsi() + ")" );
    }
}

Mixed const & Mixed::operator [] ( Mixed const & k ) const
{
    if ( this->isArray() )
    {
        return this->_pArr->operator [] (k);
    }
    else if ( this->isCollection() )
    {
        return this->_pMap->at(k);
    }
    else
    {
        throw MixedError( MixedError::meUnexpectedType, this->typeString() + " can't support " + __FUNCTION__ +  + "(" + k.toAnsi() + ") const" );
    }
}

Mixed::MixedMixedMap::value_type & Mixed::getPair( int i )
{
    if ( this->isCollection() )
    {
        return *_pMap->find( _pArr->at(i) );
    }
    else
    {
        throw MixedError( MixedError::meUnexpectedType, this->typeString() + " can't support " + __FUNCTION__ + "()" );
    }
}

Mixed::MixedMixedMap::value_type const & Mixed::getPair( int i ) const
{
    if ( this->isCollection() )
    {
        return *_pMap->find( _pArr->at(i) );
    }
    else
    {
        throw MixedError( MixedError::meUnexpectedType, this->typeString() + " can't support " + __FUNCTION__ + "()" );
    }
}

Mixed::CollectionAssigner Mixed::addPair()
{
    if ( this->_type != MT_COLLECTION )
    {
        this->createCollection();
    }
    return CollectionAssigner(this);
}

Mixed & Mixed::addPair( Mixed const & k, Mixed const & v )
{
    if ( this->isCollection() )
    {
        this->_addUniqueKey(k);
        this->_pMap->operator[](k) = v;
    }
    else
    {
        throw MixedError( MixedError::meUnexpectedType, this->typeString() + " can't support " + __FUNCTION__ + "()" );
    }
    return *this;
}

Mixed::ArrayAssigner Mixed::add()
{
    if ( this->_type != MT_ARRAY )
    {
        this->createArray();
    }
    return ArrayAssigner(this);
}

int Mixed::add( Mixed const & v )
{
    if ( this->isArray() /*|| this->isCollection()*/ )
    {
        int i = this->_pArr->size();
        this->_pArr->push_back(v);
        return i;
    }
    else
    {
        throw MixedError( MixedError::meUnexpectedType, this->typeString() + " can't support " + __FUNCTION__ + "()" );
    }
}

int Mixed::addUnique( Mixed const & v )
{
    if ( this->isArray() )
    {
        int i;
        for ( i = 0; i < (int)this->_pArr->size(); ++i )
        {
            if ( (*this->_pArr)[i] == v )
            {
                return i;
            }
        }

        i = this->_pArr->size();
        this->_pArr->push_back(v);
        return i;
    }
    else
    {
        throw MixedError( MixedError::meUnexpectedType, this->typeString() + " can't support " + __FUNCTION__ + "()" );
    }
}

void Mixed::del( Mixed const & k )
{
    if ( this->isArray() )
    {
        uint i = k;
        if ( i >= 0 && i < this->_pArr->size() )
            this->_pArr->erase( this->_pArr->begin() + i );
    }
    else if ( this->isCollection() )
    {
        MixedArray::iterator it = std::find( this->_pArr->begin(), this->_pArr->end(), k );
        if ( it != this->_pArr->end() )
            this->_pArr->erase(it);
        this->_pMap->erase(k);
    }
}

bool Mixed::has( Mixed const & ek ) const
{
    if ( this->isArray() )
    {
        return std::find( this->_pArr->begin(), this->_pArr->end(), ek ) != this->_pArr->end();
    }
    else if ( this->isCollection() )
    {
        return isset( *this->_pMap, ek );
    }
    else
    {
        return false;
    }
}

Mixed & Mixed::merge( Mixed const & v )
{
    if ( this->isArray() )
    {
        switch ( v._type )
        {
        case MT_ARRAY:
            this->_pArr->insert( this->_pArr->end(), v._pArr->begin(), v._pArr->end() );
            break;
        case MT_COLLECTION:
            {
                for ( auto itKey = v._pArr->begin(); itKey != v._pArr->end(); ++itKey )
                {
                    this->_pArr->push_back( v._pMap->at(*itKey) );
                }
            }
            break;
        default:
            this->_pArr->push_back(v);
            break;
        }
    }
    else if ( this->isCollection() )
    {
        switch ( v._type )
        {
        case MT_ARRAY:
            {
                for ( auto itVal = v._pArr->begin(); itVal != v._pArr->end(); ++itVal )
                {
                    int inx = itVal - v._pArr->begin();
                    this->_addUniqueKey(inx);
                    this->_pMap->operator[](inx) = *itVal;
                }
            }
            break;
        case MT_COLLECTION:
            {
                for ( auto itKey = v._pArr->begin(); itKey != v._pArr->end(); ++itKey )
                {
                    this->_addUniqueKey(*itKey);
                    this->_pMap->operator[](*itKey) = v._pMap->at(*itKey);
                }
            }
            break;
        default:
            {
                int i = 0;
                while ( _pMap->find(i) != _pMap->end() ) i++;
                this->_addUniqueKey(i);
                this->_pMap->operator[](i) = v;
            }
            break;
        }
    }
    else
    {
        throw MixedError( MixedError::meUnexpectedType, this->typeString() + " can't support " + __FUNCTION__ + "()" );
    }
    return *this;
}

// MT_BINARY相关 ----------------------------------------------------------------------------------------------------
void Mixed::alloc( uint size )
{
    this->free();
    this->_type = MT_BINARY;
    this->_pBuf = new Buffer();
    this->_pBuf->alloc(size);
}

bool Mixed::peekCopy()
{
    if ( this->_type == MT_BINARY && this->_pBuf != NULL )
    {
        return this->_pBuf->peekCopy();
    }
    return false;
}

int Mixed::getSize() const
{
    if ( this->isBinary() && this->_pBuf )
    {
        return this->_pBuf->getSize();
    }
    return 0;
}

void * Mixed::getBuf() const
{
    if ( this->isBinary() && this->_pBuf )
    {
        return this->_pBuf->getBuf();
    }
    return NULL;
}

// 类型解析功能 --------------------------------------------------------------------------------------------
bool Mixed::ParseBool( AnsiString const & str, bool * boolVal )
{
    if ( _stricmp( str.c_str(), "true" ) == 0 || _stricmp( str.c_str(), "yes" ) == 0 || _stricmp( str.c_str(), "on" ) == 0 )
    {
        *boolVal = true;
    }
    else if ( _stricmp( str.c_str(), "false" ) == 0 || _stricmp( str.c_str(), "no" ) == 0 || _stricmp( str.c_str(), "off" ) == 0 )
    {
        *boolVal = false;
    }
    else
    {
        ulong ul;
        ParseULong( str, &ul );
        *boolVal = ul != 0;
    }
    return true;
}

bool Mixed::ParseBool( UnicodeString const & str, bool * boolVal )
{
    if ( _wcsicmp( str.c_str(), L"true" ) == 0 || _wcsicmp( str.c_str(), L"yes" ) == 0 || _wcsicmp( str.c_str(), L"on" ) == 0 )
    {
        *boolVal = true;
    }
    else if ( _wcsicmp( str.c_str(), L"false" ) == 0 || _wcsicmp( str.c_str(), L"no" ) == 0 || _wcsicmp( str.c_str(), L"off" ) == 0 )
    {
        *boolVal = false;
    }
    else
    {
        ulong ul;
        ParseULong( str, &ul );
        *boolVal = ul != 0;
    }
    return true;
}

bool Mixed::ParseULong( AnsiString const & str, ulong * ulVal )
{
    if ( str.length() > 1 && str[0] == '0' && ( str[1] == 'x' || str[1] == 'X' ) ) // 16进制数
    {
        *ulVal = strtoul( str.c_str() + 2, NULL, 16 );
    }
    else if ( str.length() > 1 && str[0] == '0' ) // 8进制数
    {
        *ulVal = strtoul( str.c_str() + 1, NULL, 8 );
    }
    else
    {
        *ulVal = strtoul( str.c_str(), NULL, 10 );
    }
    return true;
}

bool Mixed::ParseULong( UnicodeString const & str, ulong * ulVal )
{
    if ( str.length() > 1 && str[0] == L'0' && ( str[1] == L'x' || str[1] == L'X' ) )
    {
        *ulVal = wcstoul( str.c_str() + 2, NULL, 16 );
    }
    else if ( str.length() > 1 && str[0] == L'0' )
    {
        *ulVal = wcstoul( str.c_str() + 1, NULL, 8 );
    }
    else
    {
        *ulVal = wcstoul( str.c_str(), NULL, 10 );
    }
    return true;
}

bool Mixed::ParseDouble( AnsiString const & str, double * dblVal )
{
    if ( str.length() > 1 && str[0] == '0' && ( str[1] == 'x' || str[1] == 'X' ) )
    {
        *dblVal = (double)StrToInt64( str.c_str() + 2, 16 );
    }
    else if ( str.length() > 1 && str[0] == '0' && str[1] != '.' && ( str[1] != 'e' || str[1] != 'E' ) )
    {
        *dblVal = (double)StrToInt64( str.c_str() + 1, 8 );
    }
    else
    {
        *dblVal = strtod( str.c_str(), NULL );
    }
    return true;
}

bool Mixed::ParseDouble( UnicodeString const & str, double * dblVal )
{
    if ( str.length() > 1 && str[0] == L'0' && ( str[1] == L'x' || str[1] == L'X' ) )
    {
        *dblVal = (double)StrToInt64( UnicodeToLocal( str.c_str() + 2 ), 16 );
    }
    else if ( str.length() > 1 && str[0] == L'0' && ( str[1] != L'.' || str[1] != L'e' || str[1] != L'E' ) )
    {
        *dblVal = (double)StrToInt64( UnicodeToLocal( str.c_str() + 1 ), 8 );
    }
    else
    {
        *dblVal = wcstod( str.c_str(), NULL );
    }
    return true;
}

bool Mixed::ParseUInt64( AnsiString const & str, uint64 * ui64Val )
{
    if ( str.length() > 1 && str[0] == '0' && ( str[1] == 'x' || str[1] == 'X' ) )
    {
        *ui64Val = StrToUint64( str.c_str() + 2, 16 );
    }
    else if ( str.length() > 1 && str[0] == '0' )
    {
        *ui64Val = StrToUint64( str.c_str() + 1, 8 );
    }
    else
    {
        *ui64Val = StrToUint64( str.c_str(), 10 );
    }
    return true;
}

bool Mixed::ParseUInt64( UnicodeString const & str, uint64 * ui64Val )
{
    if ( str.length() > 1 && str[0] == L'0' && ( str[1] == L'x' || str[1] == L'X' ) )
    {
        *ui64Val = StrToUint64( UnicodeToLocal( str.c_str() + 2 ), 16 );
    }

    else if ( str.length() > 1 && str[0] == L'0' )
    {
        *ui64Val = StrToUint64( UnicodeToLocal( str.c_str() + 1 ), 8 );
    }
    else
    {
        *ui64Val = StrToUint64( UnicodeToLocal(str), 10 );
    }
    return true;
}

Mixed & Mixed::ParseJson( AnsiString const & str, Mixed * val )
{
    bool r = JsonParse( str, val );
    (void)r;
    return *val;
}

String Mixed::json() const
{
    return MixedToJson( *this, false );
}

String Mixed::myJson() const
{
    return MixedToJson( *this, true );
}

Mixed & Mixed::json( String const & jsonStr )
{
    return Mixed::ParseJson( jsonStr, this );
}

// ostream 相关
WINUX_FUNC_IMPL(std::ostream &) operator << ( std::ostream & o, Mixed const & m )
{
    o << (AnsiString)m;
    return o;
}

WINUX_FUNC_IMPL(std::wostream &) operator << ( std::wostream & o, Mixed const & m )
{
    o << (UnicodeString)m;
    return o;
}


} // namespace winux
