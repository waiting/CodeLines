
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

inline static bool IsOct( char ch )
{
    return ch >= '0' && ch <= '7';
}

inline static bool IsHex( char ch )
{
    return ( ch >= '0' && ch <= '9' ) || ( ch >= 'A' && ch <= 'F' ) || ( ch >= 'a' && ch <= 'f' );
}

inline static bool IsSpace( char ch )
{
    return ch > '\0' && ch <= ' ';
}

inline static bool IsDigit( char ch )
{
    return ch >= '0' && ch <= '9';
}

inline static bool IsWord( char ch )
{
    return ( ch >= 'A' && ch <= 'Z' ) || ( ch >= 'a' && ch <= 'z' ) || ch == '_' || ((winux::byte)ch) > ((winux::byte)127U);
}

inline static char StringToChar( const char * number, int base )
{
    char * endptr;
    return (char)strtol( number, &endptr, base );
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

            *str += StringToChar( octStr.c_str(), 8 );

            break;
        }
        else if ( ch == 'x' || ch == 'X' )
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

            *str += StringToChar( hexStr.c_str(), 16 );

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

template < typename _ChTy >
/* JSON键名是否用字符串形式表示 */
static bool IsKeyNameUseString( std::basic_string<_ChTy> const & key )
{
    if ( key.empty() ) return true;
    typename std::basic_string<_ChTy>::const_iterator it;
    for ( it = key.begin(); it != key.end(); ++it )
    {
        if ( !( ( *it >= (_ChTy)'0' && *it <= (_ChTy)'9' ) || ( *it >= (_ChTy)'A' && *it <= (_ChTy)'Z' ) || ( *it >= (_ChTy)'a' && *it <= (_ChTy)'z' ) || *it == (_ChTy)'_' ) )
        {
            return true;
        }
    }
    return false;
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
            s += "\"" + AddCSlashesA( (AnsiString)v ) + "\"";
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
                s += (AnsiString)v;
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
            s += L"\"" + AddCSlashesW( (UnicodeString)v ) + L"\"";
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
                s += (UnicodeString)v;
                break;
            }
        }
    }
    return s;
}



}
