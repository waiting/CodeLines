#include "system_detection.inl"

#if defined(CL_MINGW) // for mingw
    #ifdef __STRICT_ANSI__
    #undef __STRICT_ANSI__
    #endif
#endif

#include "utilities.hpp"
#include "strings.hpp"
#include "json.hpp"
#include "time.hpp"

#include <iomanip>
#include <string.h>

#if defined(OS_WIN)

#else // OS_LINUX

    #include <unistd.h>
    #include <errno.h>

    // linux别名
    #define _stricmp strcasecmp
    #define _wcsicmp wcscasecmp
    #define _close close
    #define _open open
    #define _read read
    #define _write write

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

WINUX_FUNC_IMPL(int) Random( int n1, int n2 )
{
    static int seedInit = VoidReturnInt( srand, (unsigned int)GetUtcTimeMs() );
    (void)seedInit;
    return abs( rand() * rand() ) % ( abs( n2 - n1 ) + 1 ) + ( n1 < n2 ? 1 : -1 ) * n1;
}

////////////////////////////////////////////////////////////////////////////////////////////////

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

//inline static uint64 __StrToXQ( char const * nptr, char const * * endptr, int ibase, int flags )
//{
//    char const * p;
//    char c;
//    uint64 number;
//    uint digval;
//    uint64 maxval;
//
//    p = nptr;            /* p is our scanning pointer */
//    number = 0;            /* start with zero */
//
//    c = *p++;            /* read char */
//
//    while ( isspace((int)(unsigned char)c) )
//        c = *p++;        /* skip whitespace */
//
//    if (c == '-') {
//        flags |= FL_NEG;    /* remember minus sign */
//        c = *p++;
//    }
//    else if (c == '+')
//        c = *p++;        /* skip sign */
//
//    if (ibase < 0 || ibase == 1 || ibase > 36) {
//        /* bad base! */
//        if (endptr)
//            /* store beginning of string in endptr */
//            *endptr = nptr;
//        return 0L;        /* return 0 */
//    }
//    else if (ibase == 0) {
//        /* determine base free-lance, based on first two chars of
//           string */
//        if (c != '0')
//            ibase = 10;
//        else if (*p == 'x' || *p == 'X')
//            ibase = 16;
//        else
//            ibase = 8;
//    }
//
//    if (ibase == 16) {
//        /* we might have 0x in front of number; remove if there */
//        if (c == '0' && (*p == 'x' || *p == 'X')) {
//            ++p;
//            c = *p++;    /* advance past prefix */
//        }
//    }
//
//    /* if our number exceeds this, we will overflow on multiply */
//    maxval = _UI64_MAX / ibase;
//
//
//    for (;;) {    /* exit in middle of loop */
//        /* convert c to value */
//        if ( isdigit((int)(unsigned char)c) )
//            digval = c - '0';
//        else if ( isalpha((int)(unsigned char)c) )
//            digval = toupper(c) - 'A' + 10;
//        else
//            break;
//        if (digval >= (unsigned)ibase)
//            break;        /* exit loop if bad digit found */
//
//        /* record the fact we have read one digit */
//        flags |= FL_READDIGIT;
//
//        /* we now need to compute number = number * base + digval,
//           but we need to know if overflow occured.  This requires
//           a tricky pre-check. */
//
//        if (number < maxval || (number == maxval && (uint64)digval <= _UI64_MAX % ibase)) {
//            /* we won't overflow, go ahead and multiply */
//            number = number * ibase + digval;
//        }
//        else {
//            /* we would have overflowed -- set the overflow flag */
//            flags |= FL_OVERFLOW;
//        }
//
//        c = *p++;        /* read next digit */
//    }
//
//    --p;                /* point to place that stopped scan */
//
//    if (!(flags & FL_READDIGIT)) {
//        /* no number there; return 0 and point to beginning of
//           string */
//        if (endptr)
//            /* store beginning of string in endptr later on */
//            p = nptr;
//        number = 0L;        /* return 0 */
//    }
//    else if ( (flags & FL_OVERFLOW) ||
//              ( !(flags & FL_UNSIGNED) &&
//                ( ( (flags & FL_NEG) && (number > -_I64_MIN) ) ||
//                  ( !(flags & FL_NEG) && (number > _I64_MAX) ) ) ) )
//    {
//        /* overflow or signed overflow occurred */
//        errno = ERANGE;
//        if ( flags & FL_UNSIGNED )
//            number = _UI64_MAX;
//        else if ( flags & FL_NEG )
//            number = _I64_MIN;
//        else
//            number = _I64_MAX;
//    }
//    if (endptr != NULL)
//        /* store pointer to char that stopped the scan */
//        *endptr = p;
//
//    if (flags & FL_NEG)
//        /* negate result if there was a neg sign */
//        number = (uint64)(-(int64)number);
//
//    return number;            /* done. */
//}
//*/

// class Buffer ----------------------------------------------------------------------
Buffer::Buffer() : _buf(NULL), _dataSize(0U), _capacity(0U), _isPeek(false)
{
}

Buffer::Buffer( void const * buf, size_t size, bool isPeek ) : _buf(NULL), _dataSize(0U), _capacity(0U), _isPeek(false)
{
    this->setBuf( buf, size, isPeek );
}

Buffer::Buffer( AnsiString const & data, bool isPeek ) : _buf(NULL), _dataSize(0U), _capacity(0U), _isPeek(false)
{
    this->setBuf( data.c_str(), data.size(), isPeek );
}

Buffer::~Buffer()
{
    this->free();
}

Buffer::Buffer( Buffer const & other ) : _buf(NULL), _dataSize(0U), _capacity(0U), _isPeek(false)
{
    this->setBuf( other._buf, other._dataSize, other._isPeek );
}

Buffer & Buffer::operator = ( Buffer const & other )
{
    if ( this != &other )
    {
        this->setBuf( other._buf, other._dataSize, other._isPeek );
    }
    return *this;
}

#ifndef MOVE_SEMANTICS_DISABLED

Buffer::Buffer( Buffer && other ) :
    _buf( std::move(other._buf) ),
    _dataSize( std::move(other._dataSize) ),
    _capacity( std::move(other._capacity) ),
    _isPeek( std::move(other._isPeek) )
{
    other._buf = NULL;
    other._dataSize = 0U;
    other._capacity = 0U;
    other._isPeek = false;
}

Buffer & Buffer::operator = ( Buffer && other )
{
    if ( this != &other )
    {
        this->free();

        this->_buf = std::move(other._buf);
        this->_dataSize = std::move(other._dataSize);
        this->_capacity = std::move(other._capacity);
        this->_isPeek = std::move(other._isPeek);

        other._buf = NULL;
        other._dataSize = 0U;
        other._capacity = 0U;
        other._isPeek = false;
    }
    return *this;
}

Buffer::Buffer( GrowBuffer && other ) :
    _buf( std::move(other._buf) ),
    _dataSize( std::move(other._dataSize) ),
    _capacity( std::move(other._capacity) ),
    _isPeek( std::move(other._isPeek) )
{
    other._buf = NULL;
    other._dataSize = 0U;
    other._capacity = 0U;
    other._isPeek = false;
}

Buffer & Buffer::operator = ( GrowBuffer && other )
{
    if ( this != &other )
    {
        this->free();

        this->_buf = std::move(other._buf);
        this->_dataSize = std::move(other._dataSize);
        this->_capacity = std::move(other._capacity);
        this->_isPeek = std::move(other._isPeek);

        other._buf = NULL;
        other._dataSize = 0U;
        other._capacity = 0U;
        other._isPeek = false;
    }
    return *this;
}

#endif

void Buffer::setBuf( void const * buf, size_t size, size_t capacity, bool isPeek )
{
    this->free();
    this->_buf = const_cast<void *>(buf);
    this->_dataSize = size;
    this->_capacity = capacity;
    this->_isPeek = isPeek;
    if ( !this->_isPeek && buf != NULL ) // 如果不是窥探模式，则需要拷贝数据
    {
        this->_buf = _Alloc(this->_capacity);
        memcpy( this->_buf, buf, capacity );
    }
}

void Buffer::alloc( size_t capacity, bool setDataSize )
{
    this->free();
    if ( setDataSize ) this->_dataSize = capacity;
    this->_capacity = capacity;
    this->_isPeek = false;
    this->_buf = _Alloc(capacity); // 分配空间
    memset( this->_buf, 0, capacity );
}

void Buffer::realloc( size_t newCapacity )
{
    if ( this->_isPeek ) // 如果是窥探模式首先要变为拷贝模式
    {
        this->peekCopy(true);
    }
    this->_buf = _Realloc( this->_buf, newCapacity );
    if ( newCapacity > this->_capacity ) // 当新容量大于当前容量，则初始化大于的部分为0
    {
        memset( ((byte *)this->_buf) + this->_capacity, 0, newCapacity - this->_capacity );
    }
    this->_capacity = newCapacity;

    if ( newCapacity < this->_dataSize ) // 当新容量小于数据量，数据被丢弃
    {
        this->_dataSize = newCapacity;
    }
}

bool Buffer::peekCopy( bool copyCapacity )
{
    if ( this->_isPeek )
    {
        if ( this->_buf != NULL )
        {
            void * buf = this->_buf;
            if ( copyCapacity )
            {
                this->_buf = _Alloc(this->_capacity);
                memcpy( this->_buf, buf, this->_capacity );
            }
            else
            {
                this->_buf = _Alloc(this->_dataSize);
                memcpy( this->_buf, buf, this->_dataSize );
                this->_capacity = this->_dataSize;
            }
        }
        this->_isPeek = false;
        return true;
    }
    return false;
}

void * Buffer::detachBuf( size_t * size )
{
    void * buf = this->_buf;
    ASSIGN_PTR(size) = this->_dataSize;
    if ( this->_buf != NULL && !this->_isPeek )
    {
        this->_buf = NULL;
        this->_dataSize = 0U;
        this->_capacity = 0U;
        this->_isPeek = false;
    }
    return buf;
}

void Buffer::free()
{
    if ( this->_buf != NULL && !this->_isPeek )
    {
        _Free(this->_buf);
        this->_buf = NULL;
        this->_dataSize = 0U;
        this->_capacity = 0U;
        this->_isPeek = false;
    }
}

void * Buffer::_Alloc( size_t size )
{
    return ::malloc(size);
}

void * Buffer::_Realloc( void * p, size_t newSize )
{
    return ::realloc( p, newSize );
}

void Buffer::_Free( void * p )
{
    ::free(p);
}

// class GrowBuffer -----------------------------------------------------------------------
GrowBuffer::GrowBuffer( size_t capacity )
{
    if ( capacity )
    {
        this->alloc( capacity, false );
    }
}

GrowBuffer::GrowBuffer( GrowBuffer const & other )
{
    this->setBuf( other._buf, other._dataSize, other._capacity, other._isPeek );
}

GrowBuffer & GrowBuffer::operator = ( GrowBuffer const & other )
{
    if ( this != &other )
    {
        this->setBuf( other._buf, other._dataSize, other._capacity, other._isPeek );
    }
    return *this;
}

GrowBuffer::GrowBuffer( Buffer const & other ) : Buffer(other)
{
}

GrowBuffer & GrowBuffer::operator = ( Buffer const & other )
{
    Buffer::operator = (other);
    return *this;
}

#ifndef MOVE_SEMANTICS_DISABLED

GrowBuffer::GrowBuffer( GrowBuffer && other ) : Buffer( std::move(other) )
{
}

GrowBuffer & GrowBuffer::operator = ( GrowBuffer && other )
{
    if ( this != &other )
    {
        Buffer::operator = ( std::move(other) );
    }
    return *this;
}

GrowBuffer::GrowBuffer( Buffer && other ) : Buffer( std::move(other) )
{
}

GrowBuffer & GrowBuffer::operator = ( Buffer && other )
{
    if ( this != &other )
    {
        Buffer::operator = ( std::move(other) );
    }
    return *this;
}

#endif

inline static size_t __AutoIncrement( size_t n )
{
    return ( n + n / 3 + 3 ) / 4 * 4 + 1;
}

void GrowBuffer::append( void const * data, size_t size )
{
    if ( this->_dataSize + size > this->_capacity ) // 需要重新分配大小
    {
        this->realloc( __AutoIncrement( this->_dataSize + size ) );
    }
    memcpy( (winux::byte *)this->_buf + this->_dataSize, data, size );
    this->_dataSize += size;
}

// 1 2 3 4 5 6 7 8 9 10
// 0 1 2 3 4 5 6 7 8 9
void GrowBuffer::erase( size_t start, size_t count )
{
    if ( (offset_t)start >= 0 && start < _dataSize )
    {
        if ( count == (size_t)-1 || count >= _dataSize - start ) // 如果count=-1或者count>=数据可删除量,就全部删除
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
            double delta = _capacity / (double)_dataSize;
            if ( delta > 1.5 )
                this->realloc(_dataSize);
        }
        else
        {
            if ( _capacity - _dataSize > 100 )
            {
                this->realloc(_dataSize);
            }
        }
    }
}

// class Mixed ----------------------------------------------------------------------------

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
    else // v1._type < v2._type
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

// enum Mixed::MixedType strings ----------------------------------------------------------
static String __MixedTypeStrings[] = {
    MixedType_ENUM_ITEMLIST(MixedType_ENUM_ITEMSTRING)
};

String const & Mixed::TypeString( MixedType type )
{
    return __MixedTypeStrings[type];
}

// ----------------------------------------------------------------------------------------

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

Mixed::Mixed( char const * str, size_t len ) // 多字节字符串
{
    _zeroInit();
    this->assign( str, len );
}

Mixed::Mixed( wchar const * str, size_t len ) // Unicode字符串
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

Mixed::Mixed( void const * binaryData, size_t size, bool isPeek )
{
    _zeroInit();
    this->assign( binaryData, size, isPeek );
}

Mixed::Mixed( Mixed * arr, size_t count )
{
    _zeroInit();
    this->assign( arr, count );
}

Mixed::Mixed( std::initializer_list<Mixed> list )
{
    _zeroInit();
    this->_type = MT_ARRAY;
    this->_pArr = new MixedArray( std::move(list) );
}

Mixed::Mixed( $a arr )
{
    _zeroInit();
    this->_type = MT_ARRAY;
    this->_pArr = new MixedArray( std::move(arr._list) );
}

Mixed::Mixed( $c coll )
{
    _zeroInit();
    this->_type = MT_COLLECTION;
    this->_pArr = new MixedArray(); // 存放keys
    this->_pMap = new MixedMixedMap();
    for ( auto & pr : coll._list )
    {
        this->_addUniqueKey(pr.first);
        ( *this->_pMap )[pr.first] = pr.second;
    }
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
        memcpy( this, &other, sizeof(Mixed) );
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
        s.assign( (AnsiString::value_type*)this->_pBuf->getBuf(), this->_pBuf->getSize() / sizeof(AnsiString::value_type) );
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
        s.assign( (UnicodeString::value_type*)this->_pBuf->getBuf(), this->_pBuf->getSize() / sizeof(UnicodeString::value_type) );
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
            size_t n = this->_pArr->size();
            for ( size_t i = 0; i < n; ++i )
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
            size_t n = this->getCount();
            for ( size_t i = 0; i < n; ++i )
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

// create functions -----------------------------------------------------------------------
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

Mixed & Mixed::createArray( size_t count )
{
    if ( this->_type == MT_ARRAY )
    {
        this->_pArr->resize(count);
    }
    else
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
    }
    return  *this;
}

Mixed & Mixed::createCollection()
{
    if ( this->_type == MT_COLLECTION )
    {
        this->_pArr->clear();
        this->_pMap->clear();
    }
    else
    {
        this->free();
        this->_type = MT_COLLECTION;
        this->_pArr = new MixedArray();
        this->_pMap = new MixedMixedMap();
    }
    return *this;
}

Mixed & Mixed::createBuffer( size_t size )
{
    if ( this->_type == MT_BINARY )
    {
        this->_pBuf->alloc(size);
    }
    else
    {
        this->free();
        this->_type = MT_BINARY;
        this->_pBuf = new Buffer();
        if ( size > 0 ) this->_pBuf->alloc(size);
    }
    return *this;
}

// Array/Collection有关的操作 --------------------------------------------------------------
Mixed & Mixed::operator [] ( Mixed const & k )
{
    if ( this->isArray() )
    {
        size_t i = k;
        if ( (offset_t)i < 0 || i >= this->_pArr->size() )
            throw MixedError( MixedError::meOutOfArrayRange, Format( "Array out of bound: index:%d, size:%d", i, this->_pArr->size() ).c_str() );

        return this->_pArr->operator [] (k);
    }
    else if ( this->isCollection() )
    {
        this->_addUniqueKey(k);
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
        size_t i = k;
        if ( (offset_t)i < 0 || i >= this->_pArr->size() )
            throw MixedError( MixedError::meOutOfArrayRange, Format( "Array out of bound: index:%d, size:%d", i, this->_pArr->size() ).c_str() );

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

Mixed const & Mixed::get( Mixed const & k, Mixed const & defval ) const
{
    switch ( this->_type )
    {
    case MT_ARRAY:
        {
            size_t i = k;
            if ( (offset_t)i < 0 || i >= this->_pArr->size() )
                return defval;

            return this->_pArr->operator [] (k);
        }
        break;
    case MT_COLLECTION:
        if ( this->_pMap->find(k) != this->_pMap->end() )
        {
            return this->_pMap->at(k);
        }
        return defval;
        break;
    default:
        return defval;
        break;
    }
}

Mixed::MixedMixedMap::value_type & Mixed::getPair( size_t i )
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

Mixed::MixedMixedMap::value_type const & Mixed::getPair( size_t i ) const
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

size_t Mixed::add( Mixed const & v )
{
    if ( this->isArray() )
    {
        size_t i = this->_pArr->size();
        this->_pArr->push_back(v);
        return i;
    }
    else
    {
        throw MixedError( MixedError::meUnexpectedType, this->typeString() + " can't support " + __FUNCTION__ + "()" );
    }
}

size_t Mixed::addUnique( Mixed const & v )
{
    if ( this->isArray() )
    {
        size_t i;
        for ( i = 0; i < this->_pArr->size(); ++i )
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
        size_t i = k;
        if ( (offset_t)i >= 0 && i < this->_pArr->size() )
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
            {
                for ( auto itVal = v._pArr->begin(); itVal != v._pArr->end(); ++itVal )
                {
                    this->_pArr->push_back(*itVal);
                }
            }
            break;
        case MT_COLLECTION:
            {
                for ( auto itKey = v._pArr->begin(); itKey != v._pArr->end(); ++itKey )
                {
                    Mixed pr;
                    pr.addPair()( *itKey, v._pMap->at(*itKey) );
                    this->_pArr->push_back( std::move(pr) );
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
                    size_t inx = ( itVal - v._pArr->begin() );
                    this->_addUniqueKey(inx);
                    this->_pMap->operator[](inx) = *itVal;
                }
            }
            break;
        case MT_COLLECTION:
            {
                for ( auto itKey = v._pArr->begin(); itKey != v._pArr->end(); ++itKey )
                {
                    // 如果存在此值并且也是个容器，则继续合并
                    if ( this->_pMap->find(*itKey) != this->_pMap->end() )
                    {
                        Mixed & thisMx = this->_pMap->operator[](*itKey);
                        if ( thisMx.isContainer() )
                        {
                            thisMx.merge( v._pMap->at(*itKey) );
                        }
                        else
                        {
                            thisMx = v._pMap->at(*itKey);
                        }
                    }
                    else
                    {
                        this->_addUniqueKey(*itKey);
                        this->_pMap->operator[](*itKey) = v._pMap->at(*itKey);
                    }
                }
            }
            break;
        default:
            {
                size_t i = 0;
                while ( _pMap->find(i) != _pMap->end() ) i++;
                this->_addUniqueKey( Mixed(i).toAnsi() );
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

Mixed & Mixed::reverse()
{
    if ( this->isCollection() || this->isArray() )
    {
        int j = (int)_pArr->size() - 1;
        int i = 0;
        while ( i < j )
        {
            Mixed t = std::move( _pArr->at(i) );
            _pArr->at(i) = std::move( _pArr->at(j) );
            _pArr->at(j) =  std::move(t);

            i++;
            j--;
        }
    }
    else
    {
        throw MixedError( MixedError::meUnexpectedType, this->typeString() + " can't support " + __FUNCTION__ + "()" );
    }
    return *this;
}

// MT_BINARY相关 ----------------------------------------------------------------------------------------------------
void Mixed::alloc( size_t size, bool setDataSize )
{
    this->free();
    this->_type = MT_BINARY;
    this->_pBuf = new Buffer();
    this->_pBuf->alloc( size, setDataSize );
}

bool Mixed::peekCopy( bool copyCapacity )
{
    if ( this->_type == MT_BINARY && this->_pBuf != NULL )
    {
        return this->_pBuf->peekCopy(copyCapacity);
    }
    return false;
}

size_t Mixed::getSize() const
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

// Assignments ----------------------------------------------------------------------------
void Mixed::assign( char const * str, size_t len )
{
    if ( this->_type == MT_ANSI )
    {
        str = str ? str : "";
        if ( (ssize_t)len < 0 )
        {
            this->_pStr->assign(str);
        }
        else if ( len > 0 )
        {
            this->_pStr->assign( str, len );
        }
        else
        {
            this->_pStr->clear();
        }
    }
    else
    {
        this->free();
        str = str ? str : "";
        this->_type = MT_ANSI; // set _type as AnsiString
        if ( (ssize_t)len < 0 )
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
}

void Mixed::assign( wchar const * str, size_t len )
{
    if ( this->_type == MT_UNICODE )
    {
        str = str ? str : L"";
        this->_type = MT_UNICODE; // set _type as UnicodeString
        if ( (ssize_t)len < 0 )
        {
            this->_pWStr->assign(str);
        }
        else if ( len > 0 )
        {
            this->_pWStr->assign( str, len );
        }
        else
        {
            this->_pWStr->clear();
        }
    }
    else
    {
        this->free();
        str = str ? str : L"";
        this->_type = MT_UNICODE; // set _type as UnicodeString
        if ( (ssize_t)len < 0 )
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
}

void Mixed::assign( bool boolVal )
{
    if ( this->_type == MT_BOOLEAN )
    {
        this->_boolVal = boolVal;
    }
    else
    {
        this->free();
        this->_type = MT_BOOLEAN;
        this->_boolVal = boolVal;
    }
}

void Mixed::assign( byte btVal )
{
    if ( this->_type == MT_BYTE )
    {
        this->_btVal = btVal;
    }
    else
    {
        this->free();
        this->_type = MT_BYTE;
        this->_btVal = btVal;
    }
}

void Mixed::assign( short shVal )
{
    if ( this->_type == MT_SHORT )
    {
        this->_shVal = shVal;
    }
    else
    {
        this->free();
        this->_type = MT_SHORT;
        this->_shVal = shVal;
    }
}

void Mixed::assign( ushort ushVal )
{
    if ( this->_type == MT_USHORT )
    {
        this->_ushVal = ushVal;
    }
    else
    {
        this->free();
        this->_type = MT_USHORT;
        this->_ushVal = ushVal;
    }
}

void Mixed::assign( int iVal )
{
    if ( this->_type == MT_INT )
    {
        this->_iVal = iVal;
    }
    else
    {
        this->free();
        this->_type = MT_INT;
        this->_iVal = iVal;
    }
}

void Mixed::assign( uint uiVal )
{
    if ( this->_type == MT_UINT )
    {
        this->_uiVal = uiVal;
    }
    else
    {
        this->free();
        this->_type = MT_UINT;
        this->_uiVal = uiVal;
    }
}

void Mixed::assign( long lVal )
{
    if ( this->_type == MT_LONG )
    {
        this->_lVal = lVal;
    }
    else
    {
        this->free();
        this->_type = MT_LONG;
        this->_lVal = lVal;
    }
}

void Mixed::assign( ulong ulVal )
{
    if ( this->_type == MT_ULONG )
    {
        this->_ulVal = ulVal;
    }
    else
    {
        this->free();
        this->_type = MT_ULONG;
        this->_ulVal = ulVal;
    }
}

void Mixed::assign( float fltVal )
{
    if ( this->_type == MT_FLOAT )
    {
        this->_fltVal = fltVal;
    }
    else
    {
        this->free();
        this->_type = MT_FLOAT;
        this->_fltVal = fltVal;
    }
}

void Mixed::assign( int64 i64Val )
{
    if ( this->_type == MT_INT64 )
    {
        this->_i64Val = i64Val;
    }
    else
    {
        this->free();
        this->_type = MT_INT64;
        this->_i64Val = i64Val;
    }
}

void Mixed::assign( uint64 ui64Val )
{
    if ( this->_type == MT_UINT64 )
    {
        this->_ui64Val = ui64Val;
    }
    else
    {
        this->free();
        this->_type = MT_UINT64;
        this->_ui64Val = ui64Val;
    }
}

void Mixed::assign( double dblVal )
{
    if ( this->_type == MT_DOUBLE )
    {
        this->_dblVal = dblVal;
    }
    else
    {
        this->free();
        this->_type = MT_DOUBLE;
        this->_dblVal = dblVal;
    }
}

void Mixed::assign( Buffer const & buf )
{
    if ( this->_type == MT_BINARY )
    {
        *this->_pBuf = buf;
    }
    else
    {
        this->free();
        this->_type = MT_BINARY;
        this->_pBuf = new Buffer(buf);
    }
}

void Mixed::assign( void const * binaryData, size_t size, bool isPeek )
{
    if ( this->_type == MT_BINARY )
    {
        this->_pBuf->setBuf( binaryData, size, isPeek );
    }
    else
    {
        this->free();
        this->_type = MT_BINARY;
        this->_pBuf = new Buffer( binaryData, size, isPeek );
    }
}

void Mixed::assign( Mixed * arr, size_t count )
{
    if ( this->_type == MT_ARRAY )
    {
        this->_pArr->assign( arr, arr + count );
    }
    else
    {
        this->free();
        this->_type = MT_ARRAY;
        this->_pArr = new MixedArray( arr, arr + count );
    }
}

void Mixed::assign( std::initializer_list<Mixed> list )
{
    if ( this->_type == MT_ARRAY )
    {
        this->_pArr->assign( std::move(list) );
    }
    else
    {
        this->free();
        this->_type = MT_ARRAY;
        this->_pArr = new MixedArray( std::move(list) );
    }
}

void Mixed::assign( $a arr )
{
    if ( this->_type == MT_ARRAY )
    {
        this->_pArr->assign( std::move(arr._list) );
    }
    else
    {
        this->free();
        this->_type = MT_ARRAY;
        this->_pArr = new MixedArray( std::move(arr._list) );
    }
}

void Mixed::assign( $c coll )
{
    this->free();
    this->_type = MT_COLLECTION;
    this->_pArr = new MixedArray(); // 存放keys
    this->_pMap = new MixedMixedMap();
    for ( auto & pr : coll._list )
    {
        this->_addUniqueKey(pr.first);
        ( *this->_pMap )[pr.first] = pr.second;
    }
}

// JSON -----------------------------------------------------------------------------------
String Mixed::myJson( bool autoKeyQuotes, AnsiString const & spacer, AnsiString const & newline ) const
{
    return MixedToJsonExA( *this, autoKeyQuotes, spacer, newline );
}

String Mixed::json() const
{
    return MixedToJson( *this, false );
}

Mixed & Mixed::json( String const & jsonStr )
{
    return Mixed::ParseJson( jsonStr, this );
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
        *dblVal = (double)StrToInt64A( str.c_str() + 2, 16 );
    }
    else if ( str.length() > 1 && str[0] == '0' && str[1] != '.' && ( str[1] != 'e' || str[1] != 'E' ) )
    {
        *dblVal = (double)StrToInt64A( str.c_str() + 1, 8 );
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
        *dblVal = (double)StrToInt64W( str.c_str() + 2, 16 );
    }
    else if ( str.length() > 1 && str[0] == L'0' && ( str[1] != L'.' || str[1] != L'e' || str[1] != L'E' ) )
    {
        *dblVal = (double)StrToInt64W( str.c_str() + 1, 8 );
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
        *ui64Val = StrToUint64A( str.c_str() + 2, 16 );
    }
    else if ( str.length() > 1 && str[0] == '0' )
    {
        *ui64Val = StrToUint64A( str.c_str() + 1, 8 );
    }
    else
    {
        *ui64Val = StrToUint64A( str.c_str(), 10 );
    }
    return true;
}

bool Mixed::ParseUInt64( UnicodeString const & str, uint64 * ui64Val )
{
    if ( str.length() > 1 && str[0] == L'0' && ( str[1] == L'x' || str[1] == L'X' ) )
    {
        *ui64Val = StrToUint64W( str.c_str() + 2, 16 );
    }

    else if ( str.length() > 1 && str[0] == L'0' )
    {
        *ui64Val = StrToUint64W( str.c_str() + 1, 8 );
    }
    else
    {
        *ui64Val = StrToUint64W( str, 10 );
    }
    return true;
}

Mixed & Mixed::ParseJson( AnsiString const & str, Mixed * val )
{
    bool r = JsonParse( str, val );
    (void)r;
    return *val;
}

void Mixed::_zeroInit()
{
    memset( this, 0, sizeof(Mixed) );
    this->_type = MT_NULL;
}

// ostream 相关
WINUX_FUNC_IMPL(std::ostream &) operator << ( std::ostream & o, Mixed const & m )
{
    o << m.toAnsi();
    return o;
}

WINUX_FUNC_IMPL(std::wostream &) operator << ( std::wostream & o, Mixed const & m )
{
    o << m.toUnicode();
    return o;
}


} // namespace winux
