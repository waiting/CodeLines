#ifndef __FILESYS_HPP__
#define __FILESYS_HPP__

/** 文件系统相关封装 */
#if defined(_MSC_VER) || defined(WIN32)

#else
#include <dirent.h>
#endif

namespace winux
{

// 特殊平台变量 -------------------------------------------------------------
#if defined(_MSC_VER) || defined(WIN32)
String const dirSep = "\\"; ///< 目录分割符
String const lineSep = "\r\n"; ///< 行分割符
String const pathEnvSep = ";"; ///< PATH环境变量路径分割符
String const DirSep = "\\"; ///< 目录分割符
String const LineSep = "\r\n"; ///< 行分割符
String const PathEnvSep = ";"; ///< PATH环境变量路径分割符
#else
String const dirSep = "/"; ///< 目录分割符
String const lineSep = "\n"; ///< 行分割符
String const pathEnvSep = ":"; ///< PATH环境变量路径分割符
String const DirSep = "/"; ///< 目录分割符
String const LineSep = "\n"; ///< 行分割符
String const PathEnvSep = ":"; ///< PATH环境变量路径分割符
#endif

/** \brief 获取可执行文件的全路径 */
WINUX_FUNC_DECL(String) GetExecutablePath( void );

/** \brief 获取路径名(末尾不含目录分隔符)
 *
 * \param fullPath String const&
 * \param fileName String*
 * \return String */
WINUX_FUNC_DECL(String) FilePath( String const & fullPath, String * fileName = NULL );

/** \brief 获取文件标题
 *
 * \param fileName String const&
 * \param extName String*
 * \return String */
WINUX_FUNC_DECL(String) FileTitle( String const & fileName, String * extName = NULL );

/** \brief 判断是否为绝对路径 */
WINUX_FUNC_DECL(bool) IsAbsPath( String const & path );

/** \brief 使路径规则化(末尾不带路径分割符) */
WINUX_FUNC_DECL(String) NormalizePath( String const & path );

/** \brief 计算真实路径 */
WINUX_FUNC_DECL(String) RealPath( String const & path );

/** \brief 返回当前工作目录(末尾不含目录分隔符) */
WINUX_FUNC_DECL(String) GetCurrentDir( void );

/** \brief 设置当前工作目录 */
WINUX_FUNC_DECL(bool) SetCurrentDir( String const & path );

/** \brief 判断是否是一个目录 */
WINUX_FUNC_DECL(bool) IsDir( String const & path );

/** \brief 探测一个路径是存在还是不存在，是目录还是文件
 *
 *  返回true/false表示是否存在, *isDir返回true/false表示是否为目录 */
WINUX_FUNC_DECL(bool) DetectPath( String const & path, bool * isDir = NULL );

/** \brief 获取文件大小 */
WINUX_FUNC_DECL(ulong) FileSize( String const & filename );
/** \brief 获取文件大小（大于4GB的文件） */
WINUX_FUNC_DECL(uint64) FileSize64( String const & filename );

/** \brief 获取文件时间 */
WINUX_FUNC_DECL(bool) FileTime( String const & filename, ulong * ctime, ulong * mtime, ulong * atime );
/** \brief 获取文件创建时间 */
WINUX_FUNC_DECL(ulong) FileCTime( String const & filename );
/** \brief 获取文件修改时间 */
WINUX_FUNC_DECL(ulong) FileMTime( String const & filename );
/** \brief 获取文件访问时间 */
WINUX_FUNC_DECL(ulong) FileATime( String const & filename );

/** \brief 更新文件修改时间,访问时间 */
WINUX_FUNC_DECL(bool) FileTouch( String const & filename, ulong time = (ulong)-1, ulong atime = (ulong)-1 );

/** \brief 路径分隔符整理 */
WINUX_FUNC_DECL(String) PathWithSep( String const & path );
WINUX_FUNC_DECL(String) PathNoSep( String const & path );
WINUX_FUNC_DECL(String &) PathWithSep( String * path );
WINUX_FUNC_DECL(String &) PathNoSep( String * path );

/** \brief 把一个目录路径和一个文件名组合成一个新路径 */
WINUX_FUNC_DECL(String) CombinePath( String const & dirPath, String const & fileName );

/** \brief 获取文件夹中的文件和子文件夹,sortType:0结果不排序 1正序 2反序 */
WINUX_FUNC_DECL(void) FolderData( String const & path, StringArray * fileArr, StringArray * subFolderArr, int sortType = 0 );

/** \brief 在指定路径下枚举指定扩展名的文件
 *
 *  ext可以是扩展名串，也可以是一个数组。
 *  当isRecursive=false时，arrFiles返回的结果不用区别目录，因此不包含目录部分的路径。 */
WINUX_FUNC_DECL(ulong) EnumFiles( String const & path, Mixed const & ext, StringArray * arrFiles, bool isRecursive = false );

/** \brief 通用删除,删除文件夹和文件,返回删除的文件夹和文件数 */
WINUX_FUNC_DECL(ulong) CommonDelete( String const & path );

/** \brief 确保目录路径的存在性，如果不存在则创建。
 *
 *  linux下需设置目录权限(8进制数),默认权限0755\n
 *  owner - group - other\n
 *  R W X - R W X - R W X\n
 *  1 1 1 - 1 0 1 - 1 0 1\n
 *  7       5       5 */
WINUX_FUNC_DECL(bool) MakeDirExists( String const & path, int mode = 0755 );

#ifndef interface
#define interface struct
#endif

/** \brief 文件系统错误类 */
class WINUX_DLL FileSysError : public Error
{
public:
    enum
    {
        fseNone,
        fseNotImplemented, ///< 方法未实现
        fseFsSelfError, ///< 文件系统自身的错误
    };
    FileSysError( int errType, AnsiString const & s ) throw() : Error( errType, s ) { }
};

/** \brief 目录文件枚举器 */
class WINUX_DLL DirIterator
{
private:
    String _path;
    String _name;
#if defined(_MSC_VER) || defined(WIN32)
    WIN32_FIND_DATA _wfd;
    SimpleHandle<HANDLE> _findFile;
    bool _first;
#else
    SimpleHandle< DIR*> _findFile;
#endif
public:
    DirIterator( String const & path );
    /** \brief 取得路径 */
    String const & getPath() const { return _path; }
    /** \brief 取得文件名 */
    String const & getName() const { return _name; }
    /** \brief 取得文件全路径 */
    String getFullPath() const;
    /** \brief 取得文件真实路径 */
    String getRealPath() const;
    /** \brief 判断是否为文件夹 */
    bool isDir() const;
    /** \brief 下一个文件/文件夹 */
    bool next();

    DISABLE_OBJECT_COPY(DirIterator)
};

/** \brief 文件接口 */
interface WINUX_DLL IFile
{
    virtual ~IFile() { }

    /** \brief 打开文件 */
    virtual bool open( String const & filename, String const & mode );
    /** \brief 关闭文件 */
    virtual bool close();
    /** \brief 读数据,返回读取的字节数 */
    virtual winux::ulong read( void * buf, winux::ulong size );
    /** \brief 写数据,返回写入字节数 */
    virtual winux::ulong write( void const * data, winux::ulong size );
    virtual winux::ulong write( Buffer const & buf );
    /** \brief 重置文件指针到头 */
    virtual bool rewind();
    /** \brief 移动文件指针,offset表示偏移量 */
    virtual bool seek( long offset );
    /** \brief 获得文件指针位置 */
    virtual winux::ulong tell();
    /** \brief 获取一行字符串,包括换行符 */
    virtual String getLine();
    /** \brief 输出字符串 */
    virtual int puts( String const & str );
    /** \brief 文件是否结束 */
    virtual bool eof();
    /** \brief 文件大小 */
    virtual winux::ulong size();
    /** \brief 取得缓冲区 */
    virtual void * buffer( winux::ulong * size );
    /** \brief 取得缓冲区 */
    virtual AnsiString buffer();
};

/** \brief 文件操作类 */
class WINUX_DLL File : public IFile
{
protected:
    String _filename;
    FILE * _fp;
    bool _autoload; // 当以读取模式打开时,是否自动载入数据到缓冲区
    winux::ulong _fileSize; // 文件的字节大小,这玩意和数据加载大小不一定相同
    winux::ulong _loadedSize; // 实际加载的字节大小,这玩意和文件大小不一定相同
    Buffer _buf;

    void _loadData();
public:
    File( String const & filename, String const & mode, bool autoload = true );
    virtual ~File();

    virtual bool open( String const & filename, String const & mode );
    virtual bool close();
    virtual winux::ulong read( void * buf, winux::ulong size );
    virtual winux::ulong write( void const * data, winux::ulong size );
    virtual winux::ulong write( Buffer const & buf );
    virtual bool rewind();
    virtual bool seek( long offset );
    virtual winux::ulong tell();
    virtual String getLine();
    virtual int puts( String const & str );
    virtual bool eof();
    virtual winux::ulong size();
    virtual void * buffer( winux::ulong * size );
    virtual AnsiString buffer();

    winux::ulong loadedSize() const { return _loadedSize; }
    operator bool() const { return _fp != NULL; }

    DISABLE_OBJECT_COPY(File)
};

/** \brief 分块输出文件 */
class WINUX_DLL BlockOutFile : public File
{
protected:
    String _dirname;   ///< 目录名   path/to
    String _basename;  ///< 文件名   filename.txt
    String _filetitle; ///< 文件标题 filename
    String _extname;   ///< 扩展名   .txt
    long _fileno;      ///< 文件编号

    bool _isTextMode; ///< 是否文本模式
    winux::ulong _blockSize; ///< 块大小

public:
    BlockOutFile( String const & filename, bool isTextMode = true, winux::ulong blockSize = 1048576 );

    bool nextBlock();
    virtual winux::ulong write( void const * data, winux::ulong size );
    virtual winux::ulong write( Buffer const & buf );
    int puts( String const & str );

    DISABLE_OBJECT_COPY(BlockOutFile)
};

/** \brief 分块输入文件 */
class WINUX_DLL BlockInFile : public File
{
protected:
    String _dirname;   ///< 目录名   path/to
    String _basename;  ///< 文件名   filename.txt
    String _filetitle; ///< 文件标题 filename
    String _extname;   ///< 扩展名   .txt
    long _index;       ///< 文件索引

    bool _isTextMode;  ///< 是否文本模式
    StringArray _blockFiles; ///< 要加载的分块文件
public:
    BlockInFile( String const & filename, bool isTextMode = true );

    bool nextBlock();
    virtual bool eof();

    DISABLE_OBJECT_COPY(BlockInFile)
};

}

#endif //__FILESYS_HPP__
