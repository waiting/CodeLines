#ifndef __ARCHIVES_HPP__
#define __ARCHIVES_HPP__
//
// winux提供一些文档的读写功能
//

namespace winux
{

/** \brief 配置文件类 */
class WINUX_DLL Configure
{
private:
    String _configFile;
    StringStringMap _rawParams; //!< 未StripSlashes处理的数据集合

    static int _FindConfigRef( String const & str, int offset, int * length, String * name );
    String _expandVarNoStripSlashes( String const & name, StringArray * chains ) const;

    //返回加载的配置变量个数
    int _load( String const & configFile, StringStringMap * rawParams, StringArray * loadFileChains );
public:
    static String const ConfigVarsSlashChars;

    /** \brief 构造函数0 */
    Configure();

    /** \brief 构造函数1
     *
     *  \param configFile 配置文件的路径 */
    Configure( String const & configFile );

    /** \brief 载入配置文件，返回加载的配置变量个数。不会清空原数据 */
    int load( String const & configFile );

    /** \brief 判断是否含有该变量 */
    bool has( String const & name ) const { return _rawParams.find(name) != _rawParams.end(); }

    /** \brief 按指定方式获取变量值 */
    String get( String const & name, bool stripslashes = false, bool expand = false ) const;

    /** \brief 获取变量未展开的值 */
    String operator [] ( String const & name ) const;

    /** \brief 获取变量展开的值 */
    String operator () ( String const & name ) const;

    /** \brief 以RAW方式设置一个配置变量
     *
     * 必须是单行字符串值，特殊字符必须反转义 */
    void setRaw( String const & name, String const & value );

    /** \brief 设置一个配置变量
     *
     *  值会自动反转义，因此无法包含$(XXX)型的内部待展开变量，因为set()内部会自动反转义变成\\$\\(XXX\\)。
     *  需要设置$(XXX)型内部待展开变量的请使用setRaw()。 */
    void set( String const & name, String const & value );

    /** \brief 删除一个配置变量 */
    bool del( String const & name );

    /** \brief 清空所有配置变量 */
    void clear();

    /** \brief 取得内部StringStringMap引用 */
    StringStringMap const & getAll() const { return _rawParams; }
};

/** \brief 配置设置类 */
class WINUX_DLL ConfigureSettings
{
public:
    ConfigureSettings( String const & settingsFile = "" );
    ~ConfigureSettings();
    ConfigureSettings( ConfigureSettings const & other );
    ConfigureSettings( ConfigureSettings && other );
    ConfigureSettings & operator = ( ConfigureSettings const & other );
    ConfigureSettings & operator = ( ConfigureSettings && other );

    /** \brief 加载设置文件 */
    size_t load( String const & settingsFile );

    /** \brief 更新表达式并计算结果。（当你修改表达式后应该执行这个函数一次）
     *
     *  \param multiname 此参数不是表达式，而是一系列键名。可以用任何表达式可以识别的符号隔开（例如 > , . ），如果键名含空格应该用引号包起来。 */
    Mixed & update( String const & multiname, String const & updateExprStr = "" );

    /** \brief 以Root变量场景执行表达式并返回引用，如果不能执行则返回内部一个引用 */
    Mixed & execRef( String const & exprStr ) const;
    /** \brief 以Root变量场景执行表达式并返回值，如果不能执行则返回默认值 */
    Mixed execVal( String const & exprStr, Mixed const & defval = Mixed() ) const;

    /** \brief 获取此名字的设置（只读） */
    Mixed const & operator [] ( String const & name ) const;
    /** \brief 获取此名字的设置 */
    Mixed & operator [] ( String const & name );

    /** \brief 判断是否有此名字的设置 */
    bool has( String const & name ) const;

    /** \brief 获取此名字的设置（只读） */
    Mixed const & get( String const & name ) const;
    /** \brief 设置此名字的设置 */
    ConfigureSettings & set( String const & name, Mixed const & value );

    /** \brief 值 */
    Mixed const & val() const;
    /** \brief 值 */
    Mixed & val();

    /** \brief 表达式 */
    Mixed const & expr() const;
    /** \brief 表达式 */
    Mixed & expr();

private:

    size_t _load( String const & settingsFile, winux::Mixed * collAsVal, winux::Mixed * collAsExpr, StringArray * loadFileChains );

    MembersWrapper<struct ConfigureSettings_Data> _self;
};

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
    size_t getCount() const { return _records.getCount(); }

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


/** \brief 文本文档。可载入文本文件自动识别BOM，转换编码为指定编码

    [00 00 FE FF] UTF32BE
    [FF FE 00 00] UTF32LE
    [FE FF] UTF16BE
    [FF FE] UTF16LE
    [EF BB BF] UTF8BOM */
class WINUX_DLL TextArchive
{
public:
    enum FileEncoding
    {
        MultiByte,
        Utf8,
        Utf8Bom,
        Utf16Le,
        Utf16Be,
        Utf16 = Utf16Be,
        Utf32Le,
        Utf32Be,
        Utf32 = Utf32Be

    };

    /** \brief 构造函数1
     *
     *  \param fileEncoding 文件编码
     *  \param contentEncoding 内容编码
     *  \param mbsEncoding 多字节字符串编码 */
    TextArchive( FileEncoding fileEncoding = MultiByte, winux::AnsiString const & contentEncoding = "", winux::AnsiString const & mbsEncoding = "" ) : _fileEncoding(fileEncoding), _contentEncoding(contentEncoding), _mbsEncoding(mbsEncoding)
    {
    }

    void setFileEncoding( FileEncoding fileEncoding )
    {
        this->_fileEncoding = fileEncoding;
    }

    FileEncoding const & getFileEncoding() const
    {
        return this->_fileEncoding;
    }

    void setContentEncoding( winux::AnsiString const & contentEncoding )
    {
        this->_contentEncoding = contentEncoding;
    }

    winux::AnsiString const & getContentEncoding() const
    {
        return this->_contentEncoding;
    }

    void setMultiByteEncoding( winux::AnsiString const & mbsEncoding )
    {
        this->_mbsEncoding = mbsEncoding;
    }

    winux::AnsiString const & getMultiByteEncoding() const
    {
        return this->_mbsEncoding;
    }

    void load( winux::String const & filePath, bool isConvert, winux::AnsiString const & targetEncoding = "", winux::AnsiString const & mbsEncoding = "" )
    {
        this->load( winux::FileGetContentsEx( filePath, false ), isConvert, targetEncoding, mbsEncoding );
    }

    void load( winux::IFile * f, bool isConvert, winux::AnsiString const & targetEncoding = "", winux::AnsiString const & mbsEncoding = "" )
    {
        size_t n;
        void * buf = f->buffer(&n);
        this->load( winux::Buffer( buf, n, true ), isConvert, targetEncoding, mbsEncoding );
    }

    void load( winux::Buffer const & content, bool isConvert, winux::AnsiString const & targetEncoding = "", winux::AnsiString const & mbsEncoding = "" )
    {
        this->setMultiByteEncoding(mbsEncoding);

        size_t i = 0;
        this->_recognizeEncode( content, &i );
        this->_processContent( winux::Buffer( content.getBuf<winux::byte>() + i, content.getSize() - i, true ), isConvert, targetEncoding );
    }

    template < typename _ChTy >
    void write( winux::XString<_ChTy> const & content )
    {
        this->write( winux::Buffer( (void *)content.c_str(), content.length() * sizeof(_ChTy), true ) );
    }

    template < typename _ChTy >
    void write( winux::XString<_ChTy> const & content, winux::AnsiString const & encoding )
    {
        this->write( winux::Buffer( (void *)content.c_str(), content.length() * sizeof(_ChTy), true ), encoding );
    }

    void write( winux::Buffer const & content )
    {
        this->_pureContent.append(content);
    }

    void write( winux::Buffer const & content, winux::AnsiString const & encoding )
    {
        this->setContentEncoding(encoding);
        this->_pureContent.append(content);
    }

    void save( winux::GrowBuffer * output )
    {
        this->save( output, this->_fileEncoding );
    }

    void save( winux::IFile * f )
    {
        this->save( f, this->_fileEncoding );
    }

    void save( winux::String const & filePath )
    {
        this->save( filePath, this->_fileEncoding );
    }

    void save( winux::GrowBuffer * output, FileEncoding fileEncoding )
    {
        this->saveEx( this->_pureContent, this->_contentEncoding, output, fileEncoding );
    }

    void save( winux::IFile * f, FileEncoding fileEncoding )
    {
        this->saveEx( this->_pureContent, this->_contentEncoding, f, fileEncoding );
    }

    void save( winux::String const & filePath, FileEncoding fileEncoding )
    {
        this->saveEx( this->_pureContent, this->_contentEncoding, filePath, fileEncoding );
    }

    void saveEx( winux::Buffer const & content, winux::AnsiString const & encoding, winux::GrowBuffer * output, FileEncoding fileEncoding );

    void saveEx( winux::Buffer const & content, winux::AnsiString const & encoding, winux::IFile * f, FileEncoding fileEncoding )
    {
        winux::GrowBuffer buf;
        this->saveEx( content, encoding, &buf, fileEncoding );
        f->write(buf);
    }

    void saveEx( winux::Buffer const & content, winux::AnsiString const & encoding, winux::String const & filePath, FileEncoding fileEncoding )
    {
        winux::File file( filePath, "wb" );
        this->saveEx( content, encoding, &file, fileEncoding );
    }

    template < typename _ChTy >
    winux::XString<_ChTy> toString() const
    {
        return this->_pureContent.toString<_ChTy>();
    }

    void clear()
    {
        this->_pureContent.free();
    }

private:
    void _processContent( winux::Buffer const & content, bool isConvert, winux::AnsiString const & encoding = "" );

    void _recognizeEncode( winux::Buffer const & content, size_t * pI );

    FileEncoding _fileEncoding;
    winux::GrowBuffer _pureContent; // 纯内容
    mutable winux::AnsiString _contentEncoding; // 内容编码
    mutable winux::AnsiString _mbsEncoding; // 多字节字符编码
};

} // namespace winux

#endif // __ARCHIVES_HPP__
