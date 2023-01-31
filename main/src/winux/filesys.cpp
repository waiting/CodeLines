#include "system_detection.inl"

#if defined(OS_WIN)

#else
#define __USE_LARGEFILE64
#endif

#include "utilities.hpp"
#include "smartptr.hpp"
#include "filesys.hpp"
#include "strings.hpp"

#include <fstream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <functional>

#if defined(OS_WIN)
    #include <sys/utime.h>
    #include <direct.h>
    #include <io.h>
    #include <process.h>
#else
    #include <utime.h>
    #include <unistd.h>
    #include <errno.h>
#endif

#if defined(OS_WIN)
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
    // linux别名
    #define _stricmp strcasecmp
    #define _wcsicmp wcscasecmp
    #define _close close
    #define _open open
    #define _read read
    #define _write write
    #define _O_RDONLY O_RDONLY
    #define _O_CREAT O_CREAT
    #define _O_TRUNC O_TRUNC
    #define _O_WRONLY O_WRONLY
    #define _O_TEXT 0
    #define _O_BINARY 0
    #if defined(S_IREAD) && defined(S_IWRITE)
        #define _S_IREAD S_IREAD
        #define _S_IWRITE S_IWRITE
    #else
        #define _S_IREAD S_IRUSR
        #define _S_IWRITE S_IWUSR
    #endif
#endif

namespace winux
{

#include "is_x_funcs.inl"

inline static String::size_type __StrRFindDirSep( String const & str )
{
    for ( size_t i = str.length(); i--; )
    {
        if ( str[i] == '/' || str[i] == '\\' )
        {
            return i;
        }
    }
    return String::npos;
}

WINUX_FUNC_IMPL(String) GetExecutablePath( void )
{
    String path;
    path.resize(512);
#if defined(OS_WIN)
    GetModuleFileName( NULL, &path[0], (DWORD)path.size() );
#else
    readlink( "/proc/self/exe", &path[0], path.size() );
#endif
    return path.c_str();
}

WINUX_FUNC_IMPL(String) FilePath( String const & fullPath, String * fileName )
{
    String path;
    String::size_type pos = __StrRFindDirSep(fullPath);
    if ( pos != String::npos )
    {
        path = fullPath.substr( 0, pos );
        if ( fileName != NULL ) *fileName = fullPath.substr( pos + 1 );
    }
    else
    {
        path = TEXT("");
        if ( fileName != NULL ) *fileName = fullPath;
    }
    return path;
}

WINUX_FUNC_IMPL(String) FileTitle( String const & fileName, String * extName )
{
    String fileTitle;
    String::size_type pos = fileName.rfind('.');
    if ( pos != String::npos )
    {
        fileTitle = fileName.substr( 0, pos );
        if ( extName != NULL ) *extName = fileName.substr( pos + 1 );
    }
    else
    {
        fileTitle = fileName;
        if ( extName != NULL ) *extName = TEXT("");
    }
    return fileTitle;
}

WINUX_FUNC_IMPL(bool) IsAbsPath( String const & path )
{
#if defined(OS_WIN)
    return path.empty() ? false : ( ( path[0] == '/' || path[0] == '\\' ) || ( path.length() > 1 && path[1] == ':' ) );
#else
    return path.empty() ? false : path[0] == DirSep[0];
#endif
}

WINUX_FUNC_IMPL(String) NormalizePath( String const & path )
{
    StringArray pathSubs;
    size_t n = StrSplit( path, "/\\", &pathSubs );
    size_t i, c = n;
    for ( i = 0; i < c; )
    {
        if ( i > 0 && pathSubs[i - 1] != ".." && !IsAbsPath( pathSubs[i - 1] + DirSep ) && pathSubs[i] == ".." )
        {
            size_t k;
            for ( k = i + 1; k < c; k++ )
            {
                pathSubs[k - 2] = pathSubs[k];
            }
            c -= 2;
            --i;
        }
        else if ( pathSubs[i] == "." )
        {
            size_t k;
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
            r += DirSep;
        }
    }

    if ( r.length() > 1 && r[r.length() - 1] == DirSep[0] )
    {
        r = r.substr( 0, r.length() - 1 );
    }

    return r;
}

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
        #if defined(OS_WIN)
            currWorkDir.resize(512);
            _getcwd( &currWorkDir[0], sizeof(String::value_type) * 512 );
            return NormalizePath( currWorkDir.substr( 0, 2 ) + path );
        #else
            return NormalizePath(path);
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
            return NormalizePath( currWorkDir.c_str() + DirSep + path );
        }
    }
}

WINUX_FUNC_IMPL(String) RealPathEx( String const & path, String const & workDirAbsPath )
{
    if ( workDirAbsPath.empty() )
    {
        return RealPath(path);
    }

    if ( path.empty() )
    {
        return workDirAbsPath;
    }
    else
    {
        // 判断是绝对路径还是相对路径
        if ( path[0] == '/' || path[0] == '\\' ) // 是绝对路径，但windows上要确定逻辑盘符
        {
        #if defined(OS_WIN)
            if ( workDirAbsPath.length() > 1 && workDirAbsPath[1] == ':' ) // 工作目录绝对路径是以 X: 开头
            {
                return NormalizePath( workDirAbsPath.substr( 0, 2 ) + path );
            }
            else
            {
                return NormalizePath(path);
            }
        #else
            return NormalizePath(path);
        #endif
        }
        else if ( path.length() > 1 && path[1] == ':' ) // 是绝对路径
        {
            return NormalizePath(path);
        }
        else // 是相对路径，依据工作目录路径计算
        {
            return NormalizePath( workDirAbsPath + DirSep + path );
        }
    }
}

WINUX_FUNC_IMPL(String) GetCurrentDir( void )
{
    String::value_type * p;
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
#if defined(OS_WIN)
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
#if defined(CL_MINGW)
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
        auto ch = path[path.length() - 1];
        if ( ch != '\\' && ch != '/' ) // 末尾不是分隔符
        {
            r = path + DirSep;
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
        auto ch = path[path.length() - 1];
        if ( ch != '\\' && ch != '/' ) // 末尾不是分隔符
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
            *path += DirSep;
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
    size_t n = StrSplit( path, "/\\", &subPaths );
    size_t i;
    String existsPath;
    for ( i = 0; i < n; ++i )
    {
        String subPath = subPaths[i];
        if ( i == 0 && subPath.empty() ) // 首项为空，表明为linux平台绝对路径
        {
            existsPath += DirSep;
        }
        else if ( i == 0 && subPath.length() > 1 && subPath[1] == ':' ) // 首项长度大于1,并且第二个为':',表明为windows平台绝对路径
        {
            existsPath += subPath + DirSep;
        }
        else if ( !subPath.empty() ) // 子项不为空
        {
            existsPath += subPath;
            if ( !DetectPath(existsPath) ) // 不存在则创建
            {
            #if defined(OS_WIN)
                if ( _mkdir( existsPath.c_str() ) ) return false;
            #else
                if ( _mkdir( existsPath.c_str(), mode ) ) return false;
            #endif
            }
            if ( i != n - 1 )
                existsPath += DirSep;
        }
    }
    return true;
}

WINUX_FUNC_IMPL(AnsiString) FileGetContents( String const & filename, bool textMode )
{
    AnsiString content;
    try
    {
        SimpleHandle<int> fd( _open( filename.c_str(), _O_RDONLY | ( textMode ? _O_TEXT : _O_BINARY ) ), -1, _close );
        if ( fd )
        {
            int readBytes = 0, currRead = 0;
            char buf[4096];
            do
            {
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

WINUX_FUNC_IMPL(Buffer) FileGetContentsEx( String const & filename, bool textMode )
{
    GrowBuffer content;
    try
    {
        SimpleHandle<int> fd( _open( filename.c_str(), _O_RDONLY | ( textMode ? _O_TEXT : _O_BINARY ) ), -1, _close );
        if ( fd )
        {
            int readBytes = 0, currRead = 0;
            char buf[4096];
            do
            {
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

WINUX_FUNC_IMPL(bool) FilePutContents( String const & filename, AnsiString const & content, bool textMode )
{
    bool r = false;
    try
    {
        SimpleHandle<int> fd(
            _open(
                filename.c_str(),
                _O_CREAT | _O_TRUNC | _O_WRONLY | ( textMode ? _O_TEXT : _O_BINARY ),
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
            int writtenBytes = _write( fd.get(), content.c_str(), (uint)content.size() );
            if ( writtenBytes == (int)content.size() )
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

WINUX_FUNC_IMPL(bool) FilePutContentsEx( String const & filename, Buffer const & content, bool textMode )
{
    bool r = false;
    try
    {
        SimpleHandle<int> fd(
            _open(
                filename.c_str(),
                _O_CREAT | _O_TRUNC | _O_WRONLY | ( textMode ? _O_TEXT : _O_BINARY ),
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
            int writtenBytes = _write( fd.get(), content.getBuf(), (uint)content.getSize() );
            if ( writtenBytes == (int)content.getSize() )
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

WINUX_FUNC_IMPL(void) WriteLog( String const & s )
{
    String exeFile;
    String exePath = FilePath( GetExecutablePath(), &exeFile );
    std::ofstream out( ( exePath + DirSep + FileTitle(exeFile) + ".log" ).c_str(), std::ios_base::out | std::ios_base::app );
    //_getpid();
    time_t tt = time(NULL);
    struct tm * t = gmtime(&tt);
    char sz[32] = "";
    strftime( sz, 32, "%a, %d %b %Y %H:%M:%S GMT", t );

    out << Format( "[pid:%d]", getpid() ) << sz << " - " << AddCSlashes(s) << std::endl;
}

WINUX_FUNC_IMPL(void) WriteBinLog( void const * data, size_t size )
{
    String exeFile;
    String exePath = FilePath( GetExecutablePath(), &exeFile );
    std::ofstream out( ( exePath + DirSep + FileTitle(exeFile) + ".binlog" ).c_str(), std::ios_base::out | std::ios_base::binary | std::ios_base::app );
    out.write( (char const *)data, size );
}


// class DirIterator ---------------------------------------------------------------
#if defined(OS_WIN)
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
#if defined(OS_WIN)
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

size_t IFile::read( void * buf, size_t size )
{
    throw FileSysError( FileSysError::fseNotImplemented, "This method is not implemented" );
}

size_t IFile::write( void const * data, size_t size )
{
    throw FileSysError( FileSysError::fseNotImplemented, "This method is not implemented" );
}

size_t IFile::write( Buffer const & buf )
{
    throw FileSysError( FileSysError::fseNotImplemented, "This method is not implemented" );
}

bool IFile::rewind()
{
    throw FileSysError( FileSysError::fseNotImplemented, "This method is not implemented" );
}

bool IFile::seek( offset_t offset )
{
    throw FileSysError( FileSysError::fseNotImplemented, "This method is not implemented" );
}

size_t IFile::tell()
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

size_t IFile::size()
{
    throw FileSysError( FileSysError::fseNotImplemented, "This method is not implemented" );
}

void * IFile::buffer( size_t * size )
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

size_t File::read( void * buf, size_t size )
{
    assert( _fp != NULL );
    return fread( buf, 1, size, _fp );
}

size_t File::write( void const * data, size_t size )
{
    assert( _fp != NULL );
    return fwrite( data, 1, size, _fp );
}

size_t File::write( Buffer const & buf )
{
    return this->write( buf.getBuf(), buf.getSize() );
}

bool File::rewind()
{
    assert( _fp != NULL );
    ::rewind(_fp);
    return true;
}

bool File::seek( offset_t offset )
{
    assert( _fp != NULL );
    return !fseek( _fp, (long)offset, SEEK_SET );
}

size_t File::tell()
{
    assert( _fp != NULL );
    return ftell(_fp);
}

winux::String File::getLine()
{
    assert( _fp != NULL );
    String line;
    size_t const N = 4096;
    String::value_type sz[N];
    bool hasLineSep = false;
    do
    {
        memset( sz, 0, N * sizeof(String::value_type) );
        if ( fgets( sz, N, _fp ) )
        {
            String::size_type len = strlen(sz); // 获得读取到的字符串长度
            hasLineSep = sz[len - 1] == Literal<String::value_type>::lfChar; // 判断是否读取到换行符
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

size_t File::size()
{
    return _fileSize;
}

void * File::buffer( size_t * size )
{
    *size = _loadedSize;
    return _buf.getBuf();
}

AnsiString File::buffer()
{
    size_t len;
    AnsiString::value_type * s = (AnsiString::value_type *)this->buffer(&len);
    if ( !s || len < 1 )
    {
        return AnsiString();
    }
    return AnsiString( s, len );
}

// class BlockOutFile -------------------------------------------------------------------------
BlockOutFile::BlockOutFile( String const & filename, bool isTextMode, size_t blockSize )
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

size_t BlockOutFile::write( void const * data, size_t size )
{
    _loadedSize += size;
    if ( _loadedSize > _blockSize )
    {
        this->nextBlock();
        _loadedSize = size;
    }
    return File::write( data, size );
}

size_t BlockOutFile::write( Buffer const & buf )
{
    return this->write( buf.getBuf(), buf.getSize() );
}

int BlockOutFile::puts( String const & str )
{
    _loadedSize += (int)str.length();
    if ( _loadedSize > _blockSize )
    {
        this->nextBlock();
        _loadedSize = (int)str.length();
    }
    return File::puts(str);
}

// class BlockInFile --------------------------------------------------------------------

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
