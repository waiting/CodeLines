#if defined(_MSC_VER) || defined(WIN32)
#else
#define __USE_LARGEFILE64
#endif

#include "utilities.hpp"
#include "smartptr.hpp"
#include "filesys.hpp"
#include "strings.hpp"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <time.h>
#include <functional>

#if defined(_MSC_VER) || defined(WIN32)
#include <sys/utime.h>
#include <direct.h>
#else
#include <utime.h>
#include <unistd.h>
#include <errno.h>
#endif

#if defined(_MSC_VER) || defined(WIN32)
    #ifdef UNICODE
    #define _tgetcwd _wgetcwd
    #define _tchdir _wchdir
    #define _ttoi _wtoi
    #define _tcsrchr wcsrchr
    #else
    #define _tgetcwd _getcwd
    #define _tchdir _chdir
    #define _ttoi atoi
    #define _tcsrchr strrchr
    #endif
#else // LINUX
    #ifdef UNICODE
    #define _tgetcwd wgetcwd
    #define _tchdir wchdir
    #define _ttoi wtoi
    #define _tcsrchr wcsrchr
    #else
    #define _tgetcwd getcwd
    #define _tchdir chdir
    #define _ttoi atoi
    #define _tcsrchr strrchr
    #endif
    // 别名
    #define _getcwd getcwd
    #define _stat stat
    #define _stat64 stat64
    #define _S_IFDIR __S_IFDIR
    #define _rmdir rmdir
    #define _unlink unlink
    #define _utimbuf utimbuf
    #define _utime utime
    #define _mkdir mkdir
#endif

namespace winux
{

inline static tchar * __StrRFindDirSep( String & s )
{
    for ( int i = s.length() - 1; i >= 0; --i  )
    {
        if ( s[i] == '/' || s[i] == '\\' )
        {
            return &s[i];
        }
    }
    return NULL;
}

WINUX_FUNC_IMPL(String) GetExecutablePath( void )
{
    String path;
    path.resize(512);
#if defined(__GNUC__) && !defined(WIN32)
    readlink( "/proc/self/exe", &path[0], path.size() );
#else
    GetModuleFileName( NULL, &path[0], path.size() );
#endif
    return path.c_str();
}

WINUX_FUNC_IMPL(String) FilePath( String const & fullPath, String * fileName )
{
    String path;
    String buffer = fullPath;
    tchar * psz = __StrRFindDirSep(buffer);
    if ( psz != NULL )
    {
        *psz = 0;
        path = buffer.c_str();
        if ( fileName != NULL ) *fileName = psz + 1;
    }
    else
    {
        path = TEXT("");
        if ( fileName != NULL ) *fileName = buffer.c_str();
    }
    return path;
}

WINUX_FUNC_IMPL(String) FileTitle( String const & fileName, String * extName )
{
    String temp = fileName;
    String fileTitle;
    tchar * pszFile = temp.empty() ? (tchar *)TEXT("") : &temp[0];
    tchar * psz;
    psz = _tcsrchr( pszFile, '.' );
    if ( psz != NULL )
    {
        *psz = 0;
        fileTitle = pszFile;
        if ( extName != NULL ) *extName = psz + 1;
    }
    else
    {
        fileTitle = pszFile;
        if ( extName != NULL ) *extName = TEXT("");
    }
    return fileTitle;
}

WINUX_FUNC_IMPL(bool) IsAbsPath( String const & path )
{
#if defined(__GNUC__) && !defined(WIN32)
    return path.empty() ? false : path[0] == dirSep[0];
#else
    return path.empty() ? false : ( ( path[0] == '/' || path[0] == '\\' ) || ( path.length() > 1 && path[1] == ':' ) );
#endif
}

/** 使路径规则化(末尾不带路径分割符) */
WINUX_FUNC_IMPL(String) NormalizePath( String const & path )
{
    StringArray pathSubs;
    int n = StrSplit( path, "/\\", &pathSubs );
    int i, c = n;
    for ( i = 0; i < c; )
    {
        if ( i > 0 && pathSubs[i - 1] != ".." && !IsAbsPath( pathSubs[i - 1] + dirSep ) && pathSubs[i] == ".." )
        {
            int k;
            for ( k = i + 1; k < c; k++ )
            {
                pathSubs[k - 2] = pathSubs[k];
            }
            c -= 2;
            --i;
        }
        else if ( pathSubs[i] == "." )
        {
            int k;
            for ( k = i + 1; k < c; k++ )
            {
                pathSubs[k - 1] = pathSubs[k];
            }
            c -= 1;
        }
        else
            ++i;
    }

    String r;
    for ( i = 0; i < c; ++i )
    {
        r += pathSubs[i];
        if ( i != c - 1 )
        {
            r += dirSep;
        }
    }

    if ( r.length() > 1 && r[r.length() - 1] == dirSep[0] )
    {
        r = r.substr( 0, r.length() - 1 );
    }

    return r;
}

/** 计算真实路径 */
WINUX_FUNC_IMPL(String) RealPath( String const & path )
{
    String currWorkDir;
    if ( path.empty() )
    {
        currWorkDir.resize(512);
        _getcwd( &currWorkDir[0], sizeof(String::value_type) * 512 );
        return currWorkDir.c_str();
    }
    else
    {
        // 判断是绝对路径还是相对路径
        if ( path[0] == '/' || path[0] == '\\' ) // 是绝对路径，但windows上要确定逻辑盘符
        {
        #if defined(__GNUC__) && !defined(WIN32)
            return NormalizePath(path);
        #else
            currWorkDir.resize(512);
            _getcwd( &currWorkDir[0], sizeof(String::value_type) * 512 );
            return NormalizePath( currWorkDir.substr( 0, 2 ) + path );
        #endif
        }
        else if ( path.length() > 1 && path[1] == ':' ) // 是绝对路径
        {
            return NormalizePath(path);
        }
        else // 是相对路径，依据当前路径计算
        {
            currWorkDir.resize(512);
            _getcwd( &currWorkDir[0], sizeof(String::value_type) * 512 );
            return NormalizePath( currWorkDir.c_str() + dirSep + path );
        }
    }
}

WINUX_FUNC_IMPL(String) GetCurrentDir( void )
{
    tchar * p;
    String buf;
    int size = 128;
    do
    {
        size <<= 1;
        buf.resize( size - 1 );
        p = _tgetcwd( &buf[0], size );
    }
    while ( !p && errno == ERANGE );
    return p ? p : TEXT("");
}

WINUX_FUNC_IMPL(bool) SetCurrentDir( String const & path )
{
    return !_tchdir( path.c_str() );
}

WINUX_FUNC_IMPL(bool) IsDir( String const & path )
{
    struct _stat st = { 0 };
    int r = _stat( path.c_str(), &st );
#if defined(_MSC_VER) || defined(WIN32)
    return r == 0 && ( st.st_mode & _S_IFDIR );
#else
    return r == 0 && S_ISDIR(st.st_mode);
#endif
}

WINUX_FUNC_IMPL(bool) DetectPath( String const & path, bool * isDir /*= NULL */ )
{
    ASSIGN_PTR(isDir) = false;
    struct _stat st = { 0 };
    int r = _stat( path.c_str(), &st );
    if ( r == 0 )
        ASSIGN_PTR(isDir) = ( st.st_mode & _S_IFDIR ) != 0;
    return !( r == -1 && errno == ENOENT );
}

WINUX_FUNC_IMPL(ulong) FileSize( String const & filename )
{
    struct _stat st = { 0 };
    _stat( filename.c_str(), &st );
    return st.st_size;
}

WINUX_FUNC_IMPL(uint64) FileSize64( String const & filename )
{
#if defined(__MINGW32__)
    struct __stat64 st = { 0 };
#else
    struct _stat64 st = { 0 };
#endif
    _stat64( filename.c_str(), &st );
    return st.st_size;
}

WINUX_FUNC_IMPL(bool) FileTime( String const & filename, ulong * ctime, ulong * mtime, ulong * atime )
{
    struct _stat st = { 0 };
    bool r = 0 == _stat( filename.c_str(), &st );
    ASSIGN_PTR(ctime) = (ulong)st.st_ctime;
    ASSIGN_PTR(mtime) = (ulong)st.st_mtime;
    ASSIGN_PTR(atime) = (ulong)st.st_atime;
    return r;
}

WINUX_FUNC_IMPL(ulong) FileCTime( String const & filename )
{
    struct _stat st = { 0 };
    _stat( filename.c_str(), &st );
    return (ulong)st.st_ctime;
}

WINUX_FUNC_IMPL(ulong) FileMTime( String const & filename )
{
    struct _stat st = { 0 };
    _stat( filename.c_str(), &st );
    return (ulong)st.st_mtime;
}

WINUX_FUNC_IMPL(ulong) FileATime( String const & filename )
{
    struct _stat st = { 0 };
    _stat( filename.c_str(), &st );
    return (ulong)st.st_atime;
}

WINUX_FUNC_IMPL(bool) FileTouch( String const & filename, ulong time, ulong atime )
{
    struct _utimbuf tbuf;
    time = ( time == (ulong)-1 ? (ulong)::time(NULL) : time );
    atime = ( atime == (ulong)-1 ? time : atime );
    tbuf.modtime = time;
    tbuf.actime = atime;

    return 0 == _utime( filename.c_str(), &tbuf );
}

WINUX_FUNC_IMPL(String) PathWithSep( String const & path )
{
    String r;
    if ( !path.empty() )
    {
        if ( path[path.length() - 1] != '\\' || path[path.length() - 1] == '/' ) // 末尾不是分隔符
        {
            r = path + dirSep;
        }
        else // 末尾是分隔符
        {
            r = path;
        }
    }
    return r;
}

WINUX_FUNC_IMPL(String) PathNoSep( String const & path )
{
    String r;
    if ( !path.empty() )
    {
        if ( path[path.length() - 1] != '\\' || path[path.length() - 1] == '/' ) // 末尾不是分隔符
        {
            r = path;
        }
        else // 末尾是分隔符
        {
            r = path.substr( 0, path.length() - 1 );
        }
    }
    return r;
}

WINUX_FUNC_IMPL(String &) PathWithSep( String * path )
{
    if ( !path->empty() )
    {
        if ( (*path)[ path->length() - 1 ] != '\\' || (*path)[ path->length() - 1 ] == '/' ) // 末尾不是分隔符
        {
            *path += dirSep;
        }
    }
    return *path;
}

WINUX_FUNC_IMPL(String &) PathNoSep( String * path )
{
    if ( !path->empty() )
    {
        if ( (*path)[ path->length() - 1 ] != '\\' || (*path)[ path->length() - 1 ] == '/' ) // 末尾不是分隔符
        {
        }
        else // 末尾是分隔符
        {
            *path = path->substr( 0, path->length() - 1 );
        }
    }
    return *path;
}

WINUX_FUNC_IMPL(String) CombinePath( String const & dirPath, String const & fileName )
{
    return dirPath.empty() ? fileName : PathWithSep(dirPath) + fileName;
}

WINUX_FUNC_IMPL(void) FolderData( String const & path, StringArray * fileArr, StringArray * subFolderArr, int sortType )
{
    DirIterator iter(path);
    IF_PTR(fileArr)->clear();
    IF_PTR(subFolderArr)->clear();
    while ( iter.next() )
    {
        String const & name = iter.getName();
        if ( name == "." || name == ".." ) continue;

        if ( iter.isDir() )
        {
            IF_PTR(subFolderArr)->push_back(name);
        }
        else
        {
            IF_PTR(fileArr)->push_back(name);
        }
    }
    switch ( sortType )
    {
    case 1:
        if ( fileArr ) std::sort( fileArr->begin(), fileArr->end(), std::less<String>() );
        if ( subFolderArr ) std::sort( subFolderArr->begin(), subFolderArr->end(), std::less<String>() );
        break;
    case 2:
        if ( fileArr ) std::sort( fileArr->begin(), fileArr->end(), std::greater<String>() );
        if ( subFolderArr ) std::sort( subFolderArr->begin(), subFolderArr->end(), std::greater<String>() );
        break;
    }
}

WINUX_FUNC_IMPL(ulong) EnumFiles( String const & path, Mixed const & ext, StringArray * arrFiles, bool isRecursive )
{
    ulong filesCount = 0;
    StringArray files, dirs, exts;
    StringArray::const_iterator it;
    FolderData( path, &files, &dirs );

    if ( ext.isArray() )
    {
        ext.getArray(&exts);
    }
    else if ( !ext.isNull() )
    {
        exts.push_back(ext);
    }

    for ( it = files.begin(); it != files.end(); ++it )
    {
        String extName;
        FileTitle( *it, &extName );
        if ( exts.empty() || ValueIsInArray( exts, extName, true ) )
        {
            IF_PTR(arrFiles)->push_back( isRecursive ? CombinePath( path, *it ) : *it );
            ++filesCount;
        }
    }

    if ( isRecursive )
    {
        for ( it = dirs.begin(); it != dirs.end(); ++it )
        {
            filesCount += EnumFiles( CombinePath( path, *it ), ext, arrFiles, isRecursive );
        }
    }
    return filesCount;
}

WINUX_FUNC_IMPL(ulong) CommonDelete( String const & path )
{
    ulong deletedCount = 0;
    if ( IsDir(path) )
    {
        StringArray files, dirs;
        FolderData( path, &files, &dirs );
        StringArray::const_iterator it;
        for ( it = files.begin(); it != files.end(); ++it )
        {
            if ( _unlink( CombinePath( path, *it ).c_str() ) == 0 ) // delete file success
                deletedCount++;
        }

        for ( it = dirs.begin(); it != dirs.end(); ++it )
        {
            deletedCount += CommonDelete( CombinePath( path, *it ) );
        }

        if ( _rmdir( path.c_str() ) == 0 ) // delete dir success
            deletedCount++;
    }
    else
    {
        if ( _unlink( path.c_str() ) == 0 )
            deletedCount++;
    }
    return deletedCount;
}

WINUX_FUNC_IMPL(bool) MakeDirExists( String const & path, int mode )
{
    StringArray subPaths;
    int n = StrSplit( path, "/\\", &subPaths );
    int i;
    String existsPath;
    for ( i = 0; i < n; ++i )
    {
        String subPath = subPaths[i];
        if ( i == 0 && subPath.empty() ) // 首项为空，表明为linux平台绝对路径
        {
            existsPath += dirSep;
        }
        else if ( i == 0 && subPath.length() > 1 && subPath[1] == ':' ) // 首项长度大于1,并且第二个为':',表明为windows平台绝对路径
        {
            existsPath += subPath + dirSep;
        }
        else if ( !subPath.empty() ) // 子项不为空
        {
            existsPath += subPath;
            if ( !DetectPath(existsPath) ) // 不存在则创建
            {
            #if defined(_MSC_VER) || defined(WIN32)
                if ( _mkdir( existsPath.c_str() ) )
                    return false;
            #else
                if ( _mkdir( existsPath.c_str(), mode ) )
                    return false;
            #endif
            }
            if ( i != n - 1 )
                existsPath += dirSep;
        }
    }
    return true;
}


// class DirIterator ---------------------------------------------------------------
#if defined(_MSC_VER) || defined(WIN32)
DirIterator::DirIterator( String const & path )
: _path(path), _findFile( FindFirstFile( CombinePath( path, "*" ).c_str(), &_wfd ), INVALID_HANDLE_VALUE, FindClose ), _first(true)
{

}
#else
DirIterator::DirIterator( String const & path )
: _path(path), _findFile( opendir( path.empty() ? "." : path.c_str() ), (DIR*)NULL, closedir )
{

}
#endif

bool DirIterator::next()
{
#if defined(_MSC_VER) || defined(WIN32)
    if ( _first )
    {
        _first = false;
        if ( _findFile )
        {
            _name = _wfd.cFileName;
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        if ( FindNextFile( _findFile.get(), &_wfd ) )
        {
            _name = _wfd.cFileName;
            return true;
        }
        else
        {
            return false;
        }
    }
#else
    struct dirent * d;
    if ( !_findFile ) return false;
    if ( ( d = readdir( _findFile.get() ) ) )
    {
        _name = d->d_name;
        return true;
    }
    else
    {
        return false;
    }
#endif
}

String DirIterator::getFullPath() const
{
    return CombinePath( _path, _name );
}

String DirIterator::getRealPath() const
{
    return RealPath( this->getFullPath() );
}

bool DirIterator::isDir() const
{
    return IsDir( this->getFullPath() );
}

// interface IFile ------------------------------------------------------------------
bool IFile::open( String const & filename, String const & mode )
{
    throw FileSysError( FileSysError::fseNotImplemented, "This method is not implemented" );
}

bool IFile::close()
{
    throw FileSysError( FileSysError::fseNotImplemented, "This method is not implemented" );
}

winux::ulong IFile::read( void * buf, winux::ulong size )
{
    throw FileSysError( FileSysError::fseNotImplemented, "This method is not implemented" );
}

winux::ulong IFile::write( void const * data, winux::ulong size )
{
    throw FileSysError( FileSysError::fseNotImplemented, "This method is not implemented" );
}

winux::ulong IFile::write( Buffer const & buf )
{
    throw FileSysError( FileSysError::fseNotImplemented, "This method is not implemented" );
}

bool IFile::rewind()
{
    throw FileSysError( FileSysError::fseNotImplemented, "This method is not implemented" );
}

bool IFile::seek( long offset )
{
    throw FileSysError( FileSysError::fseNotImplemented, "This method is not implemented" );
}

winux::ulong IFile::tell()
{
    throw FileSysError( FileSysError::fseNotImplemented, "This method is not implemented" );
}

winux::String IFile::getLine()
{
    throw FileSysError( FileSysError::fseNotImplemented, "This method is not implemented" );
}

int IFile::puts( String const & str )
{
    throw FileSysError( FileSysError::fseNotImplemented, "This method is not implemented" );
}

bool IFile::eof()
{
    throw FileSysError( FileSysError::fseNotImplemented, "This method is not implemented" );
}

winux::ulong IFile::size()
{
    throw FileSysError( FileSysError::fseNotImplemented, "This method is not implemented" );
}

void * IFile::buffer( winux::ulong * size )
{
    throw FileSysError( FileSysError::fseNotImplemented, "This method is not implemented" );
}

AnsiString IFile::buffer()
{
    throw FileSysError( FileSysError::fseNotImplemented, "This method is not implemented" );
}

// class File ----------------------------------------------------------------------------
File::File( String const & filename, String const & mode, bool autoload )
: _fp(NULL), _autoload(autoload), _fileSize(0), _loadedSize(0)
{
    this->open( filename, mode );
}

File::~File()
{
    this->close();
}

void File::_loadData()
{
    if ( _fp )
    {
        _buf.alloc(_fileSize);
        _loadedSize = this->read( _buf.getBuf(), _buf.getSize() );
        this->rewind();
    }
}

bool File::open( String const & filename, String const & mode )
{
    this->close();
    _filename = filename;
    if ( !_filename.empty() )
    {
        _fp = fopen( _filename.c_str(), mode.c_str() );
        if ( mode.find('w') == String::npos )
        {
            _fileSize = FileSize(_filename);
            if ( _autoload ) this->_loadData();
        }
        return _fp != NULL;
    }
    return false;
}

bool File::close()
{
    if ( _fp != NULL )
    {
        fclose(_fp);
        _fp = NULL;
        _fileSize = 0;
        _loadedSize = 0;
        _buf.free();
        return true;
    }
    return false;
}

winux::ulong File::read( void * buf, winux::ulong size )
{
    assert( _fp != NULL );
    return fread( buf, 1, size, _fp );
}

winux::ulong File::write( void const * data, winux::ulong size )
{
    assert( _fp != NULL );
    return fwrite( data, 1, size, _fp );
}

winux::ulong File::write( Buffer const & buf )
{
    return this->write( buf.getBuf(), buf.getSize() );
}

bool File::rewind()
{
    assert( _fp != NULL );
    ::rewind(_fp);
    return true;
}

bool File::seek( long offset )
{
    assert( _fp != NULL );
    return !fseek( _fp, offset, SEEK_SET );
}

winux::ulong File::tell()
{
    assert( _fp != NULL );
    return ftell(_fp);
}

winux::String File::getLine()
{
    assert( _fp != NULL );
    String line;
    winux::ulong const N = 4096;
    String::value_type sz[N];
    bool hasLineSep = false;
    do
    {
        memset( sz, 0, N * sizeof(String::value_type) );
        if ( fgets( sz, N, _fp ) )
        {
            String::size_type len = strlen(sz); // 获得读取到的字符串长度
            hasLineSep = sz[len - 1] == '\n'; // 判断是否读取到换行符
            if ( hasLineSep )
            {
                line.append( sz, len );
                return line;
                //line.clear();
            }
            else
            {
                line.append( sz, len );
            }
        }
        else
        {
            break;
        }
    } while ( !hasLineSep );

    return line;
}

int File::puts( String const & str )
{
    assert( _fp != NULL );
    return fputs( str.c_str(), _fp );
}

bool File::eof()
{
    assert( _fp != NULL );
    return feof(_fp) != 0;
}

winux::ulong File::size()
{
    return _fileSize;
}

void * File::buffer( winux::ulong * size )
{
    *size = _loadedSize;
    return _buf.getBuf();
}

AnsiString File::buffer()
{
    winux::ulong len;
    AnsiString::value_type * s = (AnsiString::value_type *)this->buffer(&len);
    if ( !s || len < 1 )
    {
        return "";
    }
    return AnsiString( s, len );
}

// class BlockOutFile -------------------------------------------------------------------------
BlockOutFile::BlockOutFile( String const & filename, bool isTextMode, winux::ulong blockSize )
: File( "", "", false )
{
    _fileno = 1;
    _isTextMode = isTextMode;
    _blockSize = blockSize;
    // 分析文件路径
    _dirname = FilePath( filename, &_basename );
    _filetitle = FileTitle( _basename, &_extname );

    this->nextBlock(); // 第一块
}

bool BlockOutFile::nextBlock()
{
    this->close(); // 关闭先前的那块
    bool r = this->open(
        CombinePath( _dirname, _filetitle + "_" + (String)Mixed(_fileno) + "." + _extname ),
        ( _isTextMode ? "w" : "wb" )
    );
    if ( r )
    {
        _fileno++;
    }
    return r;
}

winux::ulong BlockOutFile::write( void const * data, winux::ulong size )
{
    _loadedSize += size;
    if ( _loadedSize > _blockSize )
    {
        this->nextBlock();
        _loadedSize = size;
    }
    return File::write( data, size );
}

winux::ulong BlockOutFile::write( Buffer const & buf )
{
    return this->write( buf.getBuf(), buf.getSize() );
}

int BlockOutFile::puts( String const & str )
{
    _loadedSize += str.length();
    if ( _loadedSize > _blockSize )
    {
        this->nextBlock();
        _loadedSize = str.length();
    }
    return File::puts(str);
}

// class BlockInFile --------------------------------------------------------------------
inline static bool IsDigit( char ch )
{
    return ch >= '0' && ch <= '9';
}

// 找并提取下划线数字部分
static bool __FindAndExtractDigit( String const & str, String * part1, String * partDigit )
{
    String::size_type pos;
    if ( ( pos = str.rfind('_') ) != String::npos )
    {
        *part1 = str.substr( 0, pos + 1 );
        ++pos; // skip '_'
        while ( pos < str.length() )
        {
            if ( IsDigit(str[pos]) )
            {
                *partDigit += str[pos];
            }
            else
            {
                break;
            }
            ++pos;
        }
        if ( !partDigit->empty() )
        {
            return true;
        }
    }
    return false;
}

BlockInFile::BlockInFile( String const & filename, bool isTextMode )
: File( "", "", false ), _index(0), _isTextMode(isTextMode)
{
    // 分析文件路径
    _dirname = FilePath( filename, &_basename );
    String fileTitle = FileTitle( _basename, &_extname );
    // 处理文件标题
    // 如果有分块数字,则搜索其他分块,并判断存在性
    String strMaxFileNo;
    if ( __FindAndExtractDigit( fileTitle, &_filetitle, &strMaxFileNo ) )
    {
        int maxFileNo = (int)Mixed(strMaxFileNo);
        int i;
        for ( i = 1; i <= maxFileNo; ++i )
        {
            String curFileName = CombinePath( _dirname, _filetitle + (String)Mixed(i) + "." + _extname );
            if ( DetectPath(curFileName) )
            {
                _blockFiles.push_back(curFileName);
            }
        }
        bool flag = true;
        for ( ; flag; ++i )
        {
            String curFileName = CombinePath( _dirname, _filetitle + (String)Mixed(i) + "." + _extname );
            if ( ( flag = DetectPath(curFileName) ) )
            {
                _blockFiles.push_back(curFileName);
            }
        }
    }
    else // 如果无,则自动从1开始添加并判断存在性
    {
        int i = 1;
        bool flag = true;
        _filetitle = fileTitle;
        for ( ; flag; ++i )
        {
            String curFileName = CombinePath( _dirname, _filetitle + "_" + (String)Mixed(i) + "." + _extname );
            if ( ( flag = DetectPath(curFileName) ) )
            {
                _blockFiles.push_back(curFileName);
            }
        }
    }

    this->nextBlock();
}

bool BlockInFile::nextBlock()
{
    if ( _index >= (long)_blockFiles.size() )
    {
        return false;
    }
    bool r = this->open( _blockFiles[_index], ( _isTextMode ? "r" : "rb" ) );
    if ( r )
    {
        _index++;
    }
    return r;
}

bool BlockInFile::eof()
{
    bool r = File::eof();
    if ( r )
    {
        return !this->nextBlock();
    }
    return r;
}

}
