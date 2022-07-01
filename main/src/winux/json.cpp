
#include "utilities.hpp"
#include "json.hpp"
#include "strings.hpp"
#include <vector>

namespace winux
{

enum JsonParseContext
{
    jsonData,
    jsonNumber,
    jsonString, jsonStrAntiSlashes,
    jsonArray, jsonArrElem,
    jsonObject, jsonObjAttr, jsonObjAttrKey, jsonObjAttrVal,
    jsonIdentifier,
};

bool JsonParseData( std::vector<JsonParseContext> & jpc, String const & json, int & i, Mixed * val );

#include "is_x_funcs.inl"

inline static char NumberStringToChar( const char * number, int base )
{
    char * endptr;
    return (char)strtol( number, &endptr, base );
}

inline static winux::ulong NumberStringToNumber( const char * number, int base )
{
    char * endptr;
    return (winux::ulong)strtoul( number, &endptr, base );
}

/* 解析数字 */
bool JsonParseNumber( std::vector<JsonParseContext> & jpc, String const & json, int & i, Mixed * numVal )
{
    String numStr;
    bool isFloat = false; // check include '.'
    bool isNegative = false; // check include '-'
    int bits = 0;
    while ( i < (int)json.length() )
    {
        String::value_type ch = json[i];
        if ( IsDigit(ch) )
        {
            numStr += ch;
            ++bits;
            ++i;
        }
        else if ( ch == '-' && numStr.empty() )
        {
            isNegative = true;
            numStr += '-';
            ++i;
        }
        else if ( ch == '.' && numStr.find('.') == String::npos )
        {
            isFloat = true;
            numStr += '.';
            ++i;
        }
        else
        {
            break;
        }
    }

    if ( isFloat )
    {
        numVal->free();
        numVal->_type = Mixed::MT_DOUBLE;
        Mixed::ParseDouble( numStr, &numVal->_dblVal );
    }
    else
    {
        if ( bits > 9 ) // 大于9位数就当作int64
        {
            numVal->free();
            numVal->_type = Mixed::MT_INT64;
            Mixed::ParseUInt64( numStr, (uint64 *)&numVal->_i64Val );
        }
        else
        {
            numVal->free();
            numVal->_type = Mixed::MT_INT;
            Mixed::ParseULong( numStr, (ulong *)&numVal->_iVal );
        }
    }
    return true;

}

thread_local bool __byteOrderForUtf16 = true;
WINUX_FUNC_IMPL(bool) JsonSetByteOrderForUtf16( bool isLittleEndian )
{
    auto old = __byteOrderForUtf16;
    __byteOrderForUtf16 = isLittleEndian;
    return old;
}

thread_local String __convertToCharsetForUtf16 = "";
WINUX_FUNC_IMPL(String) JsonSetConvertToCharsetForUtf16( String const & charset )
{
    auto old = __convertToCharsetForUtf16;
    __convertToCharsetForUtf16 = charset;
    return old;
}

/* 解析转义字符 */
bool JsonParseStrAntiSlashes( std::vector<JsonParseContext> & jpc, String const & json, int & i, String * str )
{
    ++i; // skip '\'
    while ( i < (int)json.length() )
    {
        String::value_type ch = json[i];
        if ( ch == 'a' )
        {
            *str += '\a';
            ++i;
            break;
        }
        else if ( ch == 'b' )
        {
            *str += '\b';
            ++i;
            break;
        }
        else if ( ch == 't' )
        {
            *str += '\t';
            ++i;
            break;
        }
        else if ( ch == 'n' )
        {
            *str += '\n';
            ++i;
            break;
        }
        else if ( ch == 'v' )
        {
            *str += '\v';
            ++i;
            break;
        }
        else if ( ch == 'f' )
        {
            *str += '\f';
            ++i;
            break;
        }
        else if ( ch == 'r' )
        {
            *str += '\r';
            ++i;
            break;
        }
        else if ( IsOct(ch) )
        {
            String octStr;
            for ( ; i < (int)json.length(); ++i )
            {
                ch = json[i];
                if ( IsOct(ch) && octStr.length() < 3 )
                {
                    octStr += ch;
                }
                else
                {
                    break;
                }
            }

            *str += NumberStringToChar( octStr.c_str(), 8 );

            break;
        }
        else if ( ( ch | 0x20 ) == 'x' ) // 解析\xHH
        {
            ++i; // skip 'x'
            String hexStr;
            for ( ; i < (int)json.length(); ++i )
            {
                ch = json[i];
                if ( IsHex(ch) && hexStr.length() < 2 )
                {
                    hexStr += ch;
                }
                else
                {
                    break;
                }
            }

            *str += NumberStringToChar( hexStr.c_str(), 16 );

            break;
        }
        else if ( ( ch | 0x20 ) == 'u' ) // 解析\uHHHH
        {
            ++i; // skip 'u'
            String hexStr;
            for ( ; i < (int)json.length(); ++i )
            {
                ch = json[i];
                if ( IsHex(ch) && hexStr.length() < 4 )
                {
                    hexStr += ch;
                }
                else
                {
                    break;
                }
            }

            winux::ulong code0 = NumberStringToNumber( hexStr.c_str(), 16 );
            UnicodeString16::value_type chars[] = { (UnicodeString16::value_type)code0, 0 };
            Conv conv( __byteOrderForUtf16 ? "UTF-16LE" : "UTF-16BE", __convertToCharsetForUtf16.c_str() );
            *str += conv.convert< AnsiString, UnicodeString16 >(chars);

            break;
        }
        else // 其余加\的字符都照原样输出
        {
            *str += ch;
            ++i;
            break;
        }
    }
    return true;
}

/* 解析字符串 */
bool JsonParseString( std::vector<JsonParseContext> & jpc, String const & json, int & i, Mixed * strVal )
{
    String::value_type quote = json[i]; // 记下是什么引号

    strVal->assign(""); // 初始化字符串

    String * str = strVal->_pStr;
    ++i; // skip quote

    while ( i < (int)json.length() )
    {
        String::value_type ch = json[i];
        if ( ch == quote )
        {
            ++i;
            break;
        }
        else if ( ch == '\\' ) // 进入转义字符解析
        {
            jpc.push_back(jsonStrAntiSlashes);
            JsonParseStrAntiSlashes( jpc, json, i, str );
            jpc.pop_back();
        }
        else
        {
            *str += ch;
            ++i;
        }
    }
    return true;
}

/* 解析数组 */
bool JsonParseArray( std::vector<JsonParseContext> & jpc, String const & json, int & i, Mixed * arrVal )
{
    ++i; // skip '['

    arrVal->createArray(); // 创建数组

    jpc.push_back(jsonArrElem);

    Mixed elem; // 解析到的数据
    bool hasElem = false; // 解析到元素

    while ( i < (int)json.length() )
    {
        String::value_type ch = json[i];
        if ( IsSpace(ch) )
        {
            ++i; // 跳过空白字符
        }
        else if ( ch == ',' ) // 另一个数组元素
        {
            ++i; // skip ','
            if ( hasElem )
            {
                arrVal->add(elem);
                hasElem = false;
            }
        }
        else if ( ch == ']' ) // 结束
        {
            ++i;
            break;
        }
        else
        {
            if ( !hasElem )
            {
                JsonParseData( jpc, json, i, &elem );
                hasElem = true;
            }
            else // 跳过多余数据
            {
                ++i;
            }
        }
    }

    if ( hasElem )
    {
        arrVal->add(elem);
        hasElem = false;
    }

    jpc.pop_back();

    return true;

}

/* 解析一个对象(map) */
bool JsonParseObject( std::vector<JsonParseContext> & jpc, String const & json, int & i, Mixed * objVal )
{
    ++i; // skip '{'
    objVal->createCollection();
    jpc.push_back(jsonObjAttr);

    Mixed key, val;
    bool hasKey = false, hasVal = false;
    bool currIsKey = true;

    while ( i < (int)json.length() )
    {
        String::value_type ch = json[i];
        if ( IsSpace(ch) )
        {
            ++i; // 跳过空白字符
        }
        else if ( ch == ',' )
        {
            ++i;
            if ( hasKey && hasVal )
            {
                (*objVal)[key] = val;
            }
            else if ( hasKey && !hasVal )
            {
                (*objVal)[key] = Mixed();
            }

            hasKey = hasVal = false;
            currIsKey = true;
        }
        else if ( ch == ':' )
        {
            ++i;
            currIsKey = false;
        }
        else if ( ch == '}' )
        {
            ++i;
            break;
        }
        else
        {
            if ( currIsKey )
            {
                jpc.push_back(jsonObjAttrKey);
                if ( ch == '\"' || ch == '\'' ) // 是字符串
                {
                    JsonParseString( jpc, json, i, &key );
                }
                else
                {
                    key.assign("");
                    for ( ; i < (int)json.length(); ++i )
                    {
                        ch = json[i];
                        if ( IsSpace(ch) || ch == ':' || ch == ',' || ch == '}' )
                        {
                            break;
                        }
                        else
                        {
                            *key._pStr += ch;
                        }
                    }
                }
                jpc.pop_back();

                hasKey = true;
            }
            else
            {
                jpc.push_back(jsonObjAttrVal);
                JsonParseData( jpc, json, i, &val );
                jpc.pop_back();
                hasVal = true;
            }
        }
    }

    if ( hasKey && hasVal )
    {
        (*objVal)[key] = val;
    }
    else if ( hasKey && !hasVal )
    {
        (*objVal)[key] = Mixed();
    }
    hasKey = hasVal = false;
    currIsKey = true;

    jpc.pop_back();

    return true;

}

/* 解析一个标识符,这个标识符可能是个常量或者未定义 */
bool JsonParseIdentifier( std::vector<JsonParseContext> & jpc, String const & json, int & i, Mixed * val )
{
    String identifier;
    while ( i < (int)json.length() )
    {
        String::value_type ch = json[i];
        if ( IsWord(ch) )
        {
            identifier += ch;
            ++i;
        }
        else
        {
            break;
        }
    }

    // 常量辨别
    if ( identifier == "false" )
    {
        *val = false;
    }
    else if ( identifier == "true" )
    {
        *val = true;
    }
    else // 其他未定义常量通通设定为MT_NULL
    {
        val->free();
    }

    return true;
}

/* 解析一个json数据 */
bool JsonParseData( std::vector<JsonParseContext> & jpc, String const & json, int & i, Mixed * val )
{
    val->free();

    while ( i < (int)json.length() )
    {
        String::value_type ch = json[i];
        if ( IsSpace(ch) ) // 是空白字符，跳过
        {
            ++i;
        }
        else if ( IsDigit(ch) || ch == '-' || ch == '.' ) // 可能是数字，进入数字解析
        {
            jpc.push_back(jsonNumber);
            JsonParseNumber( jpc, json, i, val );
            jpc.pop_back();
            return true;
            break;
        }
        else if ( ch == '\"' || ch == '\'' ) // 是字符串，进入字符串解析
        {
            jpc.push_back(jsonString);
            JsonParseString( jpc, json, i, val );
            jpc.pop_back();
            return true;
            break;
        }
        else if ( ch == '[' ) // 进入数组解析
        {
            jpc.push_back(jsonArray);
            JsonParseArray( jpc, json, i, val );
            jpc.pop_back();
            return true;
            break;
        }
        else if ( ch == '{' ) // 进入对象解析
        {
            jpc.push_back(jsonObject);
            JsonParseObject( jpc, json, i, val );
            jpc.pop_back();
            return true;
            break;
        }
        else if ( IsWord(ch) ) // 进入标识符解析
        {
            jpc.push_back(jsonIdentifier);
            JsonParseIdentifier( jpc, json, i, val );
            jpc.pop_back();
            return true;
            break;
        }
        else // 其他字符
        {
        //OTHER_CHARS:
            if ( jpc.back() == jsonArrElem )
            {
                if ( ch == ',' || ch == ']' )
                {
                    break;
                }
                else
                {
                    ++i;
                }
            }
            else if ( jpc.back() == jsonObjAttr )
            {
                if ( ch == ',' || ch == ':' || ch == '}' )
                {
                    break;
                }
                else
                {
                    ++i;
                }
            }
            else if ( jpc.back() == jsonObjAttrVal )
            {
                if ( ch == ',' || ch == '}' )
                {
                    break;
                }
                else
                {
                    ++i;
                }
            }
            else
            {
                ++i;
            }
        }
    }
    return false;
}

WINUX_FUNC_IMPL(bool) JsonParse( String const & json, Mixed * val )
{
    std::vector<JsonParseContext> jpc; // 解析场景
    jpc.push_back(jsonData);
    int i = 0; // 起始位置
    return JsonParseData( jpc, json, i, val );
}

WINUX_FUNC_IMPL(Mixed) Json( String const & json )
{
    Mixed v;
    JsonParse( json, &v );
    return v;
}

template < typename _ChTy >
/* JSON键名是否用字符串形式表示 */
inline static bool IsKeyNameUseString( std::basic_string<_ChTy> const & key )
{
    if ( key.empty() ) return true;
    typename std::basic_string<_ChTy>::const_iterator it;
    for ( it = key.begin(); it != key.end(); ++it )
    {
        if ( !( IsDigit(*it) || IsWord(*it) ) )
        {
            return true;
        }
    }
    return false;
}

static AnsiString __Recursive_MixedToJsonExA( int level, Mixed const & v, bool autoKeyQuotes, AnsiString const & spacer, AnsiString const & newline )
{
    AnsiString s;
    if ( v.isArray() )
    {
        s += "[" + ( newline.empty() || (int)v._pArr->size() == 0 ? " " : newline );
        for ( int i = 0; i < (int)v._pArr->size(); ++i )
        {
            if ( i != 0 ) s += "," + ( newline.empty() ? " " : newline );

            s += ( spacer.empty() ? "" : StrMultipleA( spacer, level + 1 ) ) + __Recursive_MixedToJsonExA( level + 1, v[i], autoKeyQuotes, spacer, newline );
        }
        s += ( newline.empty() || (int)v._pArr->size() == 0 ? " " : newline ) + ( spacer.empty() || (int)v._pArr->size() == 0 ? "" : StrMultipleA( spacer, level ) ) + "]";
    }
    else if ( v.isCollection() )
    {
        s += "{" + ( newline.empty() || (int)v._pArr->size() == 0 ? " " : newline );
        MixedArray::const_iterator it;
        for ( it = v._pArr->begin(); it != v._pArr->end(); ++it )
        {
            if ( it != v._pArr->begin() ) s += "," + ( newline.empty() ? " " : newline );
            // key
            if ( it->isString() )
            {
                AnsiString k = *it;
                s += ( spacer.empty() ? "" : StrMultipleA( spacer, level + 1 ) ) + ( autoKeyQuotes ? ( IsKeyNameUseString(k) ? "\"" + AddCSlashesA(k) + "\"" : k ) : ( "\"" + AddCSlashesA(k) + "\"" ) );
            }
            else
            {
                s += ( spacer.empty() ? "" : StrMultipleA( spacer, level + 1 ) ) + __Recursive_MixedToJsonExA( level + 1, *it, autoKeyQuotes, spacer, newline );
            }
            s += ":" + AnsiString( newline.empty() ? "" : " " );
            // value
            s += __Recursive_MixedToJsonExA( level + 1, v._pMap->at(*it), autoKeyQuotes, spacer, newline );
        }
        s += ( newline.empty() || (int)v._pArr->size() == 0 ? " " : newline ) + ( spacer.empty() || (int)v._pArr->size() == 0 ? "" : StrMultipleA( spacer, level ) ) + "}";
    }
    else
    {
        if ( v.isString() )
        {
            s += "\"" + AddCSlashesA( v.toAnsi() ) + "\"";
        }
        else
        {
            switch ( v._type )
            {
            case Mixed::MT_NULL:
                s += "null";
                break;
            case Mixed::MT_BOOLEAN:
                s += v._boolVal ? "true" : "false";
                break;
            default:
                s += v.toAnsi();
                break;
            }
        }
    }
    return s;
}

static UnicodeString __Recursive_MixedToJsonExW( int level, Mixed const & v, bool autoKeyQuotes, UnicodeString const & spacer, UnicodeString const & newline )
{
    UnicodeString s;
    if ( v.isArray() )
    {
        s += L"[" + ( newline.empty() || (int)v._pArr->size() == 0 ? L" " : newline );
        for ( int i = 0; i < (int)v._pArr->size(); ++i )
        {
            if ( i != 0 ) s += L"," + ( newline.empty() ? L" " : newline );

            s += ( spacer.empty() ? L"" : StrMultipleW( spacer, level + 1 ) ) + __Recursive_MixedToJsonExW( level + 1, v[i], autoKeyQuotes, spacer, newline );
        }
        s += ( newline.empty() || (int)v._pArr->size() == 0 ? L" " : newline ) + ( spacer.empty() || (int)v._pArr->size() == 0 ? L"" : StrMultipleW( spacer, level ) ) + L"]";
    }
    else if ( v.isCollection() )
    {
        s += L"{" + ( newline.empty() || (int)v._pArr->size() == 0 ? L" " : newline );
        MixedArray::const_iterator it;
        for ( it = v._pArr->begin(); it != v._pArr->end(); ++it )
        {
            if ( it != v._pArr->begin() ) s += L"," + ( newline.empty() ? L" " : newline );
            // key
            if ( it->isString() )
            {
                UnicodeString k = *it;
                s += ( spacer.empty() ? L"" : StrMultipleW( spacer, level + 1 ) ) + ( autoKeyQuotes ? ( IsKeyNameUseString(k) ? L"\"" + AddCSlashesW(k) + L"\"" : k ) : ( L"\"" + AddCSlashesW(k) + L"\"" ) );
            }
            else
            {
                s += ( spacer.empty() ? L"" : StrMultipleW( spacer, level + 1 ) ) + __Recursive_MixedToJsonExW( level + 1, *it, autoKeyQuotes, spacer, newline );
            }
            s += L":" + UnicodeString( newline.empty() ? L"" : L" " );
            // value
            s += __Recursive_MixedToJsonExW( level + 1, v._pMap->at(*it), autoKeyQuotes, spacer, newline );
        }
        s += ( newline.empty() || (int)v._pArr->size() == 0 ? L" " : newline ) + ( spacer.empty() || (int)v._pArr->size() == 0 ? L"" : StrMultipleW( spacer, level ) ) + L"}";
    }
    else
    {
        if ( v.isString() )
        {
            s += L"\"" + AddCSlashesW( v.toUnicode() ) + L"\"";
        }
        else
        {
            switch ( v._type )
            {
            case Mixed::MT_NULL:
                s += L"null";
                break;
            case Mixed::MT_BOOLEAN:
                s += v._boolVal ? L"true" : L"false";
                break;
            default:
                s += v.toUnicode();
                break;
            }
        }
    }
    return s;
}

WINUX_FUNC_IMPL(AnsiString) MixedToJsonExA( Mixed const & v, bool autoKeyQuotes, AnsiString const & spacer, AnsiString const & newline )
{
    return __Recursive_MixedToJsonExA( 0, v, autoKeyQuotes, spacer, newline );
}

WINUX_FUNC_IMPL(UnicodeString) MixedToJsonExW( Mixed const & v, bool autoKeyQuotes, UnicodeString const & spacer, UnicodeString const & newline )
{
    return __Recursive_MixedToJsonExW( 0, v, autoKeyQuotes, spacer, newline );
}

WINUX_FUNC_IMPL(AnsiString) MixedToJsonA( Mixed const & v, bool autoKeyQuotes )
{
    AnsiString s;
    if ( v.isArray() )
    {
        s += "[ ";
        for ( int i = 0; i < v.getCount(); ++i )
        {
            if ( i != 0 ) s += ", ";
            s += MixedToJsonA( v[i] , autoKeyQuotes );
        }
        s += " ]";
    }
    else if ( v.isCollection() )
    {
        s += "{ ";
        MixedArray::const_iterator it;
        for ( it = v._pArr->begin(); it != v._pArr->end(); ++it )
        {
            if ( it != v._pArr->begin() ) s += ", ";
            // key
            if ( it->isString() )
            {
                AnsiString k = *it;
                s += autoKeyQuotes ? ( IsKeyNameUseString(k) ? "\"" + AddCSlashesA(k) + "\"" : k ) : ( "\"" + AddCSlashesA(k) + "\"" );
            }
            else
            {
                s += MixedToJsonA( *it, autoKeyQuotes );
            }
            s += ":";
            // value
            s += MixedToJsonA( v._pMap->at(*it), autoKeyQuotes );
        }
        s += " }";
    }
    else
    {
        if ( v.isString() )
        {
            s += "\"" + AddCSlashesA( v.toAnsi() ) + "\"";
        }
        else
        {
            switch ( v._type )
            {
            case Mixed::MT_NULL:
                s += "null";
                break;
            case Mixed::MT_BOOLEAN:
                s += v._boolVal ? "true" : "false";
                break;
            default:
                s += v.toAnsi();
                break;
            }
        }
    }
    return s;
}

WINUX_FUNC_IMPL(UnicodeString) MixedToJsonW( Mixed const & v, bool autoKeyQuotes )
{
    UnicodeString s;
    if ( v.isArray() )
    {
        s += L"[ ";
        for ( int i = 0; i < v.getCount(); ++i )
        {
            if ( i != 0 ) s += L", ";
            s += MixedToJsonW( v[i], autoKeyQuotes );
        }
        s += L" ]";
    }
    else if ( v.isCollection() )
    {
        s += L"{ ";
        MixedArray::const_iterator it;
        for ( it = v._pArr->begin(); it != v._pArr->end(); ++it )
        {
            if ( it != v._pArr->begin() ) s += L", ";
            // key
            if ( it->isString() )
            {
                UnicodeString k = *it;
                s += autoKeyQuotes ? ( IsKeyNameUseString(k) ? L"\"" + AddCSlashesW(k) + L"\"" : k ): ( L"\"" + AddCSlashesW(k) + L"\"" );
            }
            else
            {
                s += MixedToJsonW( *it, autoKeyQuotes );
            }
            s += L":";
            // value
            s += MixedToJsonW( v._pMap->at(*it), autoKeyQuotes );
        }
        s += L" }";
    }
    else // 非容器类型
    {
        if ( v.isString() )
        {
            s += L"\"" + AddCSlashesW( v.toUnicode() ) + L"\"";
        }
        else
        {
            switch ( v._type )
            {
            case Mixed::MT_NULL:
                s += L"null";
                break;
            case Mixed::MT_BOOLEAN:
                s += v._boolVal ? L"true" : L"false";
                break;
            default:
                s += v.toUnicode();
                break;
            }
        }
    }
    return s;
}



}
