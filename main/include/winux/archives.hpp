#ifndef __ARCHIVES_HPP__
#define __ARCHIVES_HPP__
//************************************************************************
// 基于winux的CSV文件读取/写入
//************************************************************************

namespace winux
{

/** \brief CSV文件写入器 */
class WINUX_DLL CsvWriter
{
public:
    CsvWriter( IFile * outputFile );

    /** \brief 写入所有记录和列标头 */
    void write( Mixed const & records, Mixed const & columnHeaders = Mixed() );
    /** \brief CSV文件写入一条记录 */
    void writeRecord( Mixed const & record );

private:
    IFile * _outputFile;
};


/** \brief CSV文件读取器 */
class WINUX_DLL CsvReader
{
public:
    CsvReader( IFile * inputFile, bool hasColumnHeaders = false );
    CsvReader( String const & content, bool hasColumnHeaders = false );

    /** \brief 获取一行记录，再次[]可获得某一列 */
    Mixed const & operator [] ( int iRow ) const { return _records[iRow]; }
    /** \brief 如果从CSV中读取了列标头，那可以通过列标头名获取一行记录中的某一列 */
    Mixed const & operator () ( int iRow, String const & name ) const { return _records[iRow][ _columns[name] ]; }
    /** \brief 获取读取到的记录数 */
    int getCount() const { return _records.getCount(); }

    /** \brief 获取所有记录的引用，可直接操作 */
    Mixed & getRecords() { return _records; }
    /** \brief 获取所有列标头 */
    Mixed & getColumnHeaders() { return _columns; }
    /** \brief 解析CSV数据，hasColumnHeaders表示CSV中第一行是否为列标头行 */
    void read( String const & content, bool hasColumnHeaders = false );

private:
    Mixed _columns; // 第一行列名代表的索引
    Mixed _records;

    void _readRecord( String const & str, int & i, Mixed & record );

    void _readString( String const & str, int & i, String & valStr );

};

typedef winux::ulong ZRESULT;
// These are the zresult codes:
#define ZR_OK         0x00000000     // nb. the pseudo-code zr-recent is never returned,
#define ZR_RECENT     0x00000001     // but can be passed to FormatZipMessage.
// The following come from general system stuff (e.g. files not openable)
#define ZR_GENMASK    0x0000FF00
#define ZR_NODUPH     0x00000100     // couldn't duplicate the handle
#define ZR_NOFILE     0x00000200     // couldn't create/open the file
#define ZR_NOALLOC    0x00000300     // failed to allocate some resource
#define ZR_WRITE      0x00000400     // a general error writing to the file
#define ZR_NOTFOUND   0x00000500     // couldn't find that file in the zip
#define ZR_MORE       0x00000600     // there's still more data to be unzipped
#define ZR_CORRUPT    0x00000700     // the zipfile is corrupt or not a zipfile
#define ZR_READ       0x00000800     // a general error reading the file
#define ZR_PASSWORD   0x00001000     // we didn't get the right password to unzip the file
// The following come from mistakes on the part of the caller
#define ZR_CALLERMASK 0x00FF0000
#define ZR_ARGS       0x00010000     // general mistake with the arguments
#define ZR_NOTMMAP    0x00020000     // tried to ZipGetMemory, but that only works on mmap zipfiles, which yours wasn't
#define ZR_MEMSIZE    0x00030000     // the memory size is too small
#define ZR_FAILED     0x00040000     // the thing was already failed when you called this function
#define ZR_ENDED      0x00050000     // the zip creation has already been closed
#define ZR_MISSIZE    0x00060000     // the indicated input file size turned out mistaken
#define ZR_PARTIALUNZ 0x00070000     // the file had already been partially unzipped
#define ZR_ZMODE      0x00080000     // tried to mix creating/opening a zip 
// The following come from bugs within the zip library itself
#define ZR_BUGMASK    0xFF000000
#define ZR_NOTINITED  0x01000000     // initialisation didn't work
#define ZR_SEEK       0x02000000     // trying to seek in an unseekable file
#define ZR_NOCHANGE   0x04000000     // changed its mind on storage, but not allowed
#define ZR_FLATE      0x05000000     // an internal error in the de/inflation code

/** \brief ZIP压缩 */
class WINUX_DLL Zip
{
public:
    static String Message( ZRESULT code = ZR_RECENT );

    Zip();
    Zip( String const & filename, char const * password = NULL );
    Zip( void * buf, uint32 size, char const * password = NULL );
    ~Zip();

    bool create( String const & filename, char const * password = NULL );
    bool create( void * buf, uint32 size, char const * password = NULL );
    ZRESULT close();

    ZRESULT addFile( String const & dstPathInZip, String const & srcFilename );
    ZRESULT addFile( String const & dstPathInZip, void * src, uint32 size );
    ZRESULT addFolder( String const & dstPathInZip );

    void zipAll( String const & dirPath );

    ZRESULT getMemory( void * * buf, unsigned long * size );

private:
    MembersWrapper<struct Zip_Data> _self;

    DISABLE_OBJECT_COPY(Zip)
};

/** \brief ZIP解压缩 */
class WINUX_DLL Unzip
{
public:
    typedef struct ZipEntry
    {
        int index;                 // index of this file within the zip
        String name;      // filename within the zip
        winux::ulong attr;         // attributes, as in GetFileAttributes.
#if defined(__linux__) || ( defined(__GNUC__) && !defined(_WIN32) )
        time_t atime,ctime,mtime;  // access, create, modify filetimes
#else
        FILETIME atime,ctime,mtime;// access, create, modify filetimes
#endif
        long comp_size;            // sizes of item, compressed and uncompressed. These
        long unc_size;             // may be -1 if not yet known (e.g. being streamed in)
    } ZipEntry;

public:
    static String Message( ZRESULT code = ZR_RECENT );

    Unzip();
    Unzip( String const & filename, char const * password = NULL );
    Unzip( void * zipbuf, uint32 size, char const * password = NULL );
    ~Unzip();

    bool open( String const & filename, char const * password = NULL );
    bool open( void * zipbuf, uint32 size, char const * password = NULL );
    ZRESULT close();

    int getEntriesCount() const;

    ZRESULT getEntry( int index, ZipEntry * entry );

    ZRESULT findEntry( String const & name, bool caseInsensitive, int * index, ZipEntry * entry );

    ZRESULT unzipEntry( int index, String const & outFilename );
    ZRESULT unzipEntry( int index, void * buf, uint32 size );
    void unzipAll( String const & dirPath = "" );

    ZRESULT setUnzipBaseDir( String const & dirPath );

private:
    MembersWrapper<struct Unzip_Data> _self;

    DISABLE_OBJECT_COPY(Unzip)
};

}

#endif // __ARCHIVES_HPP__
