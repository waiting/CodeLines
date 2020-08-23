#ifndef __STRINGS_HPP__
#define __STRINGS_HPP__

#include <stdarg.h>

namespace winux
{

/** \brief 字符串分割
 *
 *  delimList指示出分割符列表，分割符只能是一个字符，函数会把str内分割符间的内容(即使是空串)添加到arr中，返回个数。\n
 *  当alwaysRetOneElem==true时，即使str是空串时也会向arr返回一个数组元素，元素值是空串。
 *  \param str String const&
 *  \param delimList String const&
 *  \param arr std::vector<String>*
 *  \return int */
WINUX_FUNC_DECL(int) StrSplit( String const & str, String const & delimList, StringArray * arr, bool alwaysRetOneElem = false );

/** \brief 字符串分割2
 *
 *  delim指示出分割字符串，函数会把str内分割字符串间的内容(即使是空串)添加到arr中，返回个数。\n
 *  当alwaysRetOneElem==true时，即使str是空串时也会向arr返回一个数组元素，元素值是空串。
 *  \param str String const&
 *  \param delimList String const&
 *  \param arr std::vector<String>*
 *  \return int */
WINUX_FUNC_DECL(int) StrSplit2( String const & str, String const & delim, StringArray * arr, bool alwaysRetOneElem = false );

/** \brief 字符串组合连接 */
WINUX_FUNC_DECL(String) StrJoin( String const & delim, StringArray const & arr );
/** \brief 在指定位置插入字符串 */
WINUX_FUNC_DECL(String) StrInsert( String const & str, int start, int end, String const & insert );
/** \brief 使字符串全文替换 */
WINUX_FUNC_DECL(String &) StrMakeReplace( String * str, String const & findText, String const & replaceText, String::size_type offset = 0 );

/** \brief 祛除字符串首尾出现的空白字符
 *
 * \param str String const&
 * \return String */
WINUX_FUNC_DECL(String) StrTrim( String const & str );

/** \brief 使字符串大写 */
WINUX_FUNC_DECL(String &) StrMakeUpper( String * str );
WINUX_FUNC_DECL(String) StrUpper( String const & str );
/** \brief 使字符串小写 */
WINUX_FUNC_DECL(String &) StrMakeLower( String * str );
WINUX_FUNC_DECL(String) StrLower( String const & str );

/** \brief 反斜杠操作 */
WINUX_FUNC_DECL(AnsiString) AddSlashesA( AnsiString const & str, AnsiString const & charlist );
inline AnsiString AddCSlashesA( AnsiString const & str ) { return AddSlashesA( str, "\n\r\t\v\a\\\'\"" ); }

WINUX_FUNC_DECL(UnicodeString) AddSlashesW( UnicodeString const & str, UnicodeString const & charlist );
inline UnicodeString AddCSlashesW( UnicodeString const & str ) { return AddSlashesW( str, L"\n\r\t\v\a\\\'\"" ); }

#if defined(_UNICODE) || defined(UNICODE)
    #define AddSlashes AddSlashesW
    #define AddCSlashes AddCSlashesW
#else
    #define AddSlashes AddSlashesA
    #define AddCSlashes AddCSlashesA
#endif

/** \brief 反斜杠操作 */
WINUX_FUNC_DECL(AnsiString) StripSlashes( AnsiString const & str, AnsiString const & charlist );
inline AnsiString StripCSlashes( AnsiString const & str ) { return StripSlashes( str, "\n\r\t\v\a\\\'\"" ); }

/** \brief double引号 */
WINUX_FUNC_DECL(String) AddQuotes( String const & str, tchar quote = '\"' );

/** \brief 获取字符串中的一行,支持unix，windows，mac平台的行分隔方式\n
    line 不包含换行符，i 指示起始位置，并获得处理到哪个位置。

    如何统一处理3平台的文本文件：
    最好的办法是在该平台用文本模式打开该平台产生的文本文件，然后操作。
    然而，现实不像想象的那么美好，多数情况下是处理不同平台下的文本文件，譬如在unix系平台下处理windows或mac的文本文件。
    由于行分隔不同，导致处理有一定困难。
    此函数正为此而存在。首先，你需要用二进制模式打开文件，然后读取全部数据，调用此函数取行即可。*/
WINUX_FUNC_DECL(bool) StrGetLine( String * line, String const & str, int * i, String * nl = NULL );

/** \brief KMP匹配算法 */
WINUX_FUNC_DECL(std::vector<int> &) KmpCalcNext( char const * substr, int sublen, std::vector<int> * next );
WINUX_FUNC_DECL(int) KmpMatchEx( char const * str, int len, char const * substr, int sublen, int pos, std::vector<int> const & next );
WINUX_FUNC_DECL(int) KmpMatch( char const * str, int len, char const * substr, int sublen, int pos );


/** \brief 用来使得String能够用operator<<来赋值 */
class WINUX_DLL OutStringStreamWrapper
{
    String * _str;
    std::ostringstream * _sout;
    bool _isAppend;
public:
    /** \brief 构造函数
     *
     *  \param str 要赋值的字符串
     *  \param isAppend 是否用“添加”的模式,默认是false */
    OutStringStreamWrapper( String * str, bool isAppend = false );
    OutStringStreamWrapper( OutStringStreamWrapper const & other );
    ~OutStringStreamWrapper();
    OutStringStreamWrapper & operator = ( OutStringStreamWrapper & other );

    template < typename _AnyType >
    std::ostream & operator << ( _AnyType t )
    {
        assert( _sout != NULL );
        return *_sout << t;
    }
};

/** \brief KMP多项匹配/替换 */
class WINUX_DLL MultiMatch
{
public:
    typedef std::vector<short> KmpNextValueArray;

    struct MatchState
    {
        int j; ///< 下一次从子串开始的位置
        int markpos;///< 标记，表示进行到这个位置了，该从这个位置开始
    };

    typedef std::vector<MatchState> MatchStates;

    struct MatchResult
    {
        int pos;    ///< 匹配到的位置
        int item;   ///< 匹配项的索引
    };
    /// 替换函数的类型
    typedef String (* ReplaceFuncType)( MultiMatch const * matchObj, int item, void * extra );

    /** \brief 构造函数，要求匹配项和替换项 */
    MultiMatch( String matches[], uint m, String replaces[], uint n );
    /** \brief 构造函数，要求匹配项和替换函数，若不进行替换，则replaceFunc可为NULL */
    MultiMatch( String matches[], uint count, ReplaceFuncType replaceFunc, void * extra = NULL );
    /** \brief 构造函数，要求匹配项和替换项 */
    MultiMatch( StringArray const & matches, StringArray const & replaces );
    /** \brief 构造函数，要求匹配项和替换函数，若不进行替换，则replaceFunc可为NULL */
    MultiMatch( StringArray const & matches, ReplaceFuncType replaceFunc, void * extra = NULL );

    /** \brief 构造函数
     *
     *  之后需要自己调用addMatchReplacePair()、addMatch()、setReplaceFunc()设置相关参数 */
    MultiMatch();
    void init( StringArray const & matches, StringArray const & replaces );
    void init( StringArray const & matches, ReplaceFuncType replaceFunc, void * extra = NULL );

    /** \brief 添加一对匹配替换项,返回要匹配的项数 */
    int addMatchReplacePair( String const & match, String const & replace );
    /** \brief 添加要匹配项 */
    int addMatch( String const & match );
    /** \brief 设置新的替换回调函数，返回旧的替换回调函数 */
    ReplaceFuncType setReplaceFunc( ReplaceFuncType newReplaceFunc, void * extra = NULL );

    /** \brief 搜索任意一项匹配（KMP）
     *  \param str 字符串
     *  \param offset 偏移,表示从哪个位置开始搜
     *  \return MatchResult(pos,item) 返回的pos考虑了offset值 */
    MatchResult search( String const & str, int offset = 0 ) const;
    /** \brief 非KMP算法，仅是内部算法不同，返回值和参数同search() */
    MatchResult commonSearch( String const & str, int offset = 0 ) const;
    /* 贪婪模式 */
    //MatchResult greedSearch( String const & str, int offset = 0 ) const;
    /** \brief 替换,搜索str中的matches,并替换成replaces
     *  \param str 目标字符串
     *  \param fnSearch 算法选择，请指定函数（search,commonSearch） */
    String replace( String const & str, MatchResult ( MultiMatch:: * fnSearch )( String const & str, int offset ) const = &MultiMatch::search ) const;

    String const & getMatchItem( int item ) const;
    String getReplaceItem( int item ) const;
    void setReplaceItem( int item, String const & replace );
    /** \brief 清空匹配项和替换项数据 */
    void clear();
private:
    std::vector<KmpNextValueArray> _nextVals;
    StringArray _matchItems;
    StringArray _replaceItems;
    ReplaceFuncType _replaceFunc;
    void * _extra;
};

/** \brief 设置locale信息 */
class WINUX_DLL SetLocale
{
    AnsiString _loc;
    AnsiString _prevLoc;
    static AnsiString _clsLc;
public:
    static void Set( char const * lc ) { _clsLc = lc; }
    static char const * Get() { return _clsLc.c_str(); }
    SetLocale( char const * lc = NULL );
    ~SetLocale();
    DISABLE_OBJECT_COPY(SetLocale)
};

/** \brief 返回一个本地字符串里有多少个实际的字符(by local CodePage),用于mbstowcs */
WINUX_FUNC_DECL(uint) LocalCharsCount( LocalString const & local );
/** \brief 返回一个unicode字符串转换为多字节字符串最少需要多少字节(by local CodePage),用于wcstombs */
WINUX_FUNC_DECL(uint) UnicodeMinLength( UnicodeString const & unicode );
/** \brief Unicode转换到本地Ansi */
WINUX_FUNC_DECL(LocalString) UnicodeToLocal( UnicodeString const & unicode );
/** \brief 本地Ansi转到Unicode */
WINUX_FUNC_DECL(UnicodeString) LocalToUnicode( LocalString const & local );

/** \brief 兼容字符串与Unicode,Local字符串相互转换 */
WINUX_FUNC_DECL(String) LocalToString( LocalString const & local );
WINUX_FUNC_DECL(String) UnicodeToString( UnicodeString const & unicode );
WINUX_FUNC_DECL(LocalString) StringToLocal( String const & str );
WINUX_FUNC_DECL(UnicodeString) StringToUnicode( String const & str );

/** \brief 格式化字符串 */
WINUX_DLL String FormatExV( uint cch, tchar const * fmt, va_list args );
WINUX_DLL String FormatEx( uint cch, tchar const * fmt, ... );
WINUX_DLL String Format( tchar const * fmt, ... );

/** \brief 填充based-zero字符串缓冲区包装类 */
class WINUX_DLL SZInput
{
    union
    {
        char * _psz;
        wchar_t * _pwsz;
    };
    enum { szCharInput, szWCharInput } _type;
    size_t _count;
public:
    explicit SZInput( char * psz, size_t count ) : _psz(psz), _type(szCharInput), _count(count) { }
    explicit SZInput( wchar_t * pwsz, size_t count ) : _pwsz(pwsz), _type(szWCharInput), _count(count) { }
    SZInput & operator = ( char const * pstr );
    SZInput & operator = ( wchar_t const * pwstr );

};

#if defined(__GNUC__) || _MSC_VER >= 1600
/* VC2010以上支持模板取数组大小 */
template < typename _CHAR, uint _N >
SZInput SZ( _CHAR (&sz)[_N] )
{
    return SZInput( sz, _N );
}

#else
/* 否则使用宏定义 */
#define SZ(sz) SZInput( sz, sizeof(sz) / sizeof(sz[0]) )

#endif
/* 如果操作对象是缓冲区指针，则使用SZP宏 */
#define SZP SZInput

// ----------------------------------------------------------------------------------

/** \brief 字符串编码转换 */
class WINUX_DLL Conv
{
public:
    Conv( char const * fromCode, char const * toCode );
    ~Conv();

    /** \brief 进行编码转换
     *
     *  \param srcBuf 需要转换的字符串缓冲区
     *  \param srcSize 缓冲区的大小(in bytes)
     *  \param destBuf 转换得到的结果,函数自动分配内存,用户负责free()释放
     *  \return int 输出到destBuf的字节数 */
    int convert( char const * srcBuf, size_t srcSize, char * * destBuf );

    /** \brief 进行编码转换
     *
     *  \param _RetString 返回类型
     *  \param _String 要转换的字符串类型 */
    template < typename _RetString, typename _String >
    _RetString convert( _String const & str )
    {
        typename _RetString::pointer buf;
        size_t outBytes = this->convert( (char *)str.c_str(), (str.length() + 1) * sizeof(typename _String::value_type), (char **)&buf );
        _RetString s = (typename _RetString::pointer)buf;
        free(buf);
        return s;
    }
private:
    MembersWrapper<struct Conv_Data> _self;

    DISABLE_OBJECT_COPY(Conv)
};

/** \brief 本地编码转到指定编码 */
template < typename _ToString >
class ConvTo : public Conv
{
public:
    ConvTo( char const * toCode ) : Conv( "", toCode )
    {
    }

    _ToString convert( AnsiString const & str )
    {
        return this->Conv::convert<_ToString, AnsiString>(str);
    }

    _ToString operator () ( AnsiString const & str )
    {
        typename _ToString::value_type * buf;
        Conv::convert( (char *)str.c_str(), (str.length() + 1) * sizeof(typename AnsiString::value_type), (char **)&buf );
        _ToString s = buf;
        free(buf);
        return s;
    }
};

/** \brief 指定编码转到本地编码 */
template < typename _FromString >
class ConvFrom : public Conv
{
public:
    ConvFrom( char const * fromCode ) : Conv( fromCode, "" )
    {
    }

    AnsiString convert( _FromString const & str )
    {
        return this->Conv::convert<AnsiString, _FromString>(str);
    }

    AnsiString operator () ( _FromString const & str )
    {
        AnsiString::value_type * buf;
        Conv::convert( (char *)str.c_str(), (str.length() + 1) * sizeof(typename _FromString::value_type), (char **)&buf );
        AnsiString s = buf;
        free(buf);
        return s;
    }

};

// UTF-8编码转换
/** \brief 从utf-8转到本地编码 */
WINUX_FUNC_DECL(AnsiString) LocalFromUtf8( AnsiString const & str );
/** \brief 从本地编码转到utf-8 */
WINUX_FUNC_DECL(AnsiString) LocalToUtf8( AnsiString const & str );


} // namespace winux

#endif // __STRINGS_HPP__
