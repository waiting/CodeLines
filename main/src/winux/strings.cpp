#ifndef __GNUC__
#pragma warning( disable: 4786 )
#endif

#include "utilities.hpp"
#include "strings.hpp"

#if defined(__GNUC__) || defined(HAVE_ICONV)
#include <iconv.h>
#endif

#if defined(_MSC_VER) || defined(WIN32) // IS_WINDOWS
#include <mbstring.h>
#include <tchar.h>
    #if defined(__GNUC__) // for mingw
    _CRTIMP size_t __cdecl __MINGW_NOTHROW _mbslen(const unsigned char*);
    _CRTIMP int __cdecl __MINGW_NOTHROW swprintf (wchar_t*, const wchar_t*, ...);
    #endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <algorithm>

#if defined(__GNUC__) && !defined(WIN32) // IS_LINUX
#include <errno.h>
#endif

#if defined(_MSC_VER) || defined(WIN32)
    #ifdef UNICODE
    #define _vsntprintf _vsnwprintf
    #else
    #define _vsntprintf _vsnprintf
    #endif

#else
    #ifdef UNICODE
    #define _vsntprintf vsnwprintf
    #define _tcsstr wcsstr
    #else
    #define _vsntprintf vsnprintf
    #define _tcsstr strstr
    #endif

    #define swprintf_s swprintf
#endif

namespace winux
{

inline static bool InCharList( tchar ch, String const & charlist )
{
    return charlist.find(ch) != String::npos;
}

WINUX_FUNC_IMPL(int) StrSplit( String const & str, String const & delimList, StringArray * arr, bool alwaysRetOneElem )
{
    if ( !alwaysRetOneElem && str.empty() ) return 0;

    int count = 0;
    tchar const * pstr = str.c_str();
    String tmp;

    while ( *pstr )
    {
        if ( !InCharList( *pstr, delimList ) )
        {
            tmp += *pstr;
            pstr++;
        }
        else
        {
            pstr++;
            arr->push_back(tmp);
            count++;
            tmp = TEXT("");
        }
    }
    arr->push_back(tmp);
    count++;
    return count;
}

WINUX_FUNC_IMPL(int) StrSplit2( String const & str, String const & delim, StringArray * arr, bool alwaysRetOneElem )
{
    if ( !alwaysRetOneElem && str.empty() ) return 0;
    if ( delim.empty() )
    {
        arr->push_back(str);
        return 1;
    }

    int count = 0;
    String::value_type const * cur = str.c_str();
    String::value_type const * found = cur;

    while ( ( found = _tcsstr( cur, delim.c_str() ) ) != NULL )
    {
        arr->push_back( str.substr( cur - str.c_str(), found - cur ) );
        count++;
        cur = found + delim.length();
    }
    arr->push_back( str.substr( cur - str.c_str() ) );
    count++;
    return count;
}

WINUX_FUNC_IMPL(String) StrJoin( String const & delim, StringArray const & arr )
{
    String res;
    int count = arr.size();
    int i;
    for ( i = 0; i < count; i++ )
    {
        if ( i != 0 )
        {
            res += delim;
        }
        res += arr[i];
    }
    return res;
}

WINUX_FUNC_IMPL(String) StrInsert( String const & str, int start, int end, String const & insert )
{
    String res;
    res += str.substr( 0, start );
    res += insert;
    if ( end < (int)str.length() )
        res += str.substr(end);
    return res;
}

WINUX_FUNC_IMPL(String &) StrMakeReplace( String * str, String const & findText, String const & replaceText, String::size_type offset )
{
    String::size_type pos = offset;
    while ( ( pos = str->find( findText, pos ) ) != String::npos )
    {
        str->replace( pos, findText.length(), replaceText );
        pos += replaceText.length();
    }
    return *str;
}

WINUX_FUNC_IMPL(String) StrTrim( String const & str )
{
    tchar const * p1, * p2;
    p1 = str.c_str();
    p2 = str.c_str() + str.length();
    while ( p1 < p2 && ( *p1 == ' ' || *p1 == '\r' || *p1 == '\n' || *p1 == '\t' || *p1 == '\v' ) )
    {
        p1++;
    }
    while ( p2 > p1 && ( p2[-1] == ' ' || p2[-1] == '\r' || p2[-1] == '\n' || p2[-1] == '\t' || p2[-1] == '\v' ) )
    {
        p2--;
    }

    return String( p1, p2 );
}

WINUX_FUNC_IMPL(String &) StrMakeUpper( String * str )
{
    std::transform( str->begin(), str->end(), str->begin(), toupper );
    return *str;
}

WINUX_FUNC_IMPL(String) StrUpper( String const & str )
{
    String r = str;
    StrMakeUpper(&r);
    return r;
}

WINUX_FUNC_IMPL(String &) StrMakeLower( String * str )
{
    std::transform( str->begin(), str->end(), str->begin(), tolower );
    return *str;
}

WINUX_FUNC_IMPL(String) StrLower( String const & str )
{
    String r = str;
    StrMakeLower(&r);
    return r;
}

// add/strip slashes internal functions ------------------------------------------------------------------
template < typename _ChTy >
inline static bool IsOct( _ChTy ch )
{
    return ch >= (_ChTy)'0' && ch <= (_ChTy)'7';
}
template < typename _ChTy >
inline static bool IsSpecial( _ChTy ch )
{
    return ch == (_ChTy)'a' || ch == (_ChTy)'b' || ch == (_ChTy)'t' || ch == (_ChTy)'n' || ch == (_ChTy)'v' || ch == (_ChTy)'f' || ch == (_ChTy)'r';
}
inline static bool IsHex( char ch )
{
    return ( ch >= '0' && ch <= '9' ) || ( ch >= 'A' && ch <= 'F' ) || ( ch >= 'a' && ch <= 'f' );
}

inline static char StringToChar( const char * number, int base )
{
    char * endptr;
    return (char)strtol( number, &endptr, base );
}

inline static char SpecialToChar( char special )
{
    switch ( special )
    {
    case 'a':// 响铃(BEL)  07H/7
        special = '\a';
        break;
    case 'b':// 退格符(BS) 08H/8
        special = '\b';
        break;
    case 't':// 水平制表符(HT) 09H/9
        special = '\t';
        break;
    case 'n':// 换行符(LF) 0AH/10
        special = '\n';
        break;
    case 'v':// 垂直制表(VT) 0BH/11
        special = '\v';
        break;
    case 'f':// 换页符(FF) 0CH/12
        special = '\f';
        break;
    case 'r':// 回车符(CR) 0DH/13
        special = '\r';
        break;
    }
    return special;
}

WINUX_FUNC_IMPL(AnsiString) AddSlashesA( AnsiString const & str, AnsiString const & charlist )
{
    AnsiString slashes = "";
    for ( AnsiString::const_iterator it = str.begin(); it != str.end(); it++ )
    {
        char ch = *it;
        if ( charlist.find(ch) != AnsiString::npos )
        {
            AnsiString slash = "";
            switch ( ch )
            {
            case '\a':// 响铃(BEL)  07H/7
                slash = "\\a";
                break;
            case '\b':// 退格符(BS) 08H/8
                slash = "\\b";
                break;
            case '\t':// 水平制表符(HT) 09H/9
                slash = "\\t";
                break;
            case '\n':// 换行符(LF) 0AH/10
                slash = "\\n";
                break;
            case '\v':// 垂直制表(VT) 0BH/11
                slash = "\\v";
                break;
            case '\f':// 换页符(FF) 0CH/12
                slash = "\\f";
                break;
            case '\r':// 回车符(CR) 0DH/13
                slash = "\\r";
                break;
            default:
                {
                    if ( IsSpecial(ch) || ch <= ' ' || ch & (unsigned char)0x80 )
                    {
                        char s[8] = { 0 };
                        sprintf( s, "\\x%02x", (unsigned)( ch & 0xFF ) );
                        slash = s;
                    }
                    else
                    {
                        slash += '\\';
                        slash += ch;
                    }
                }
                break;
            }
            slashes += slash;
        }
        else
        {
            slashes += ch;
        }
    }
    return slashes;
}

WINUX_FUNC_IMPL(UnicodeString) AddSlashesW( UnicodeString const & str, UnicodeString const & charlist )
{
    UnicodeString slashes = L"";
    for ( UnicodeString::const_iterator it = str.begin(); it != str.end(); it++ )
    {
        wchar ch = *it;
        if ( charlist.find(ch) != UnicodeString::npos )
        {
            UnicodeString slash = L"";
            switch ( ch )
            {
            case L'\a':// 响铃(BEL)  07H/7
                slash = L"\\a";
                break;
            case L'\b':// 退格符(BS) 08H/8
                slash = L"\\b";
                break;
            case L'\t':// 水平制表符(HT) 09H/9
                slash = L"\\t";
                break;
            case L'\n':// 换行符(LF) 0AH/10
                slash = L"\\n";
                break;
            case L'\v':// 垂直制表(VT) 0BH/11
                slash = L"\\v";
                break;
            case L'\f':// 换页符(FF) 0CH/12
                slash = L"\\f";
                break;
            case L'\r':// 回车符(CR) 0DH/13
                slash = L"\\r";
                break;
            default:
                {
                    if ( IsSpecial(ch) || ch <= L' ' || ch > (ushort)0x7f )
                    {
                        wchar s[8] = { 0 };
                        #if defined(__MINGW32__)
                        swprintf( s, L"\\x%04x", (unsigned)( ch & 0xFFFF ) );
                        #else
                        swprintf_s( s, 8, L"\\x%04x", (unsigned)( ch & 0xFFFF ) );
                        #endif
                        slash = s;
                    }
                    else
                    {
                        slash += L'\\';
                        slash += ch;
                    }
                }
                break;
            }
            slashes += slash;
        }
        else
        {
            slashes += ch;
        }
    }
    return slashes;
}

WINUX_FUNC_IMPL(AnsiString) StripSlashes( AnsiString const & str, AnsiString const & charlist )
{
    AnsiString result = "";
    for ( AnsiString::const_iterator it = str.begin(); it != str.end(); )
    {
        const char current = *it;
        if ( current == '\\' )
        {
            it++; // skip '\\'
            if ( it != str.end() ) // \后有字符
            {
                for ( ; it != str.end(); it++ )
                {
                    char ch0 = *it; // ch0表示\后的一个字符
                    if ( IsOct(ch0) )
                    {
                        AnsiString ch0s = "";
                        for ( ; it != str.end(); it++ )
                        {
                            ch0 = *it;
                            if ( IsOct(ch0) && ch0s.length() < 3 )
                            {
                                ch0s += ch0;
                            }
                            else
                            {
                                break;
                            }
                        }
                        if ( ch0s.length() > 0 )
                        {
                            char c1 = StringToChar( ch0s.c_str(), 8 );
                            if ( InCharList( c1, charlist.c_str() ) )
                            {
                                result += c1;
                            }
                            else
                            {
                                result += '\\';
                                result += ch0s;
                            }
                        }

                        break;
                    }
                    else if ( ch0 == 'x' ) // is x 16进制
                    {
                        it++; // skip 'x'
                        if ( it != str.end() )
                        {
                            if ( IsHex(*it) )
                            {
                                AnsiString ch0s = "";
                                for ( ; it != str.end(); it++ )
                                {
                                    ch0 = *it;
                                    if ( IsHex(ch0) && ch0s.length() < 2 )
                                    {
                                        ch0s += ch0;
                                    }
                                    else
                                    {
                                        break;
                                    }
                                }

                                if ( ch0s.length() > 0 )
                                {
                                    char c2 = StringToChar( ch0s.c_str(), 16 );
                                    if ( InCharList( c2, charlist.c_str() ) )
                                    {
                                        result += c2;
                                        break;
                                    }
                                    else
                                    {
                                        result += '\\';
                                        result += 'x';
                                        result += ch0s;
                                        break;
                                    }
                                }
                                break;
                            }
                            else // x后不是16进制字符
                            {
                                result += '\\';
                                result += 'x';
                                break;
                            }
                        }
                        else // 后面已经没有字符
                        {
                            result += '\\';
                            result += 'x';
                            break;
                        }
                    }
                    else if ( IsSpecial(ch0) )
                    {
                        char special = 0;
                        special = SpecialToChar(ch0);
                        // 查看CharList是否含有此字符
                        if ( InCharList( special, charlist.c_str() ) )
                        {
                            result += special;
                            it++; // skip ch0
                            break;
                        }
                        else
                        {
                            result += '\\';
                            result += ch0;
                            it++; // skip ch0
                            break;
                        }
                    }
                    else if ( InCharList( ch0, charlist.c_str() ) )
                    {
                        result += ch0;
                        it++; // skip ch0
                        break;
                    }
                    else // 都不是
                    {
                        result += '\\';
                        break;
                    }
                } // end for loop
            }
            else // \后没有字符
            {
                result += current;
            }
        }
        else // 当前字符不是 \ .
        {
            result += current;
            it++;
        }
    }
    return result;
}

WINUX_FUNC_IMPL(String) AddQuotes( String const & str, tchar quote /*= '\"' */ )
{
    String res;
    String::const_iterator it;
    for ( it = str.begin(); it != str.end(); ++it )
    {
        if ( *it == quote )
        {
            res += String( 1, quote ) + *it;
        }
        else
        {
            res += *it;
        }
    }
    return res;
}

WINUX_FUNC_IMPL(bool) StrGetLine( String * line, String const & str, int * i, String * nl )
{
    if ( *i >= (int)str.length() )
    {
        return false;
    }

    IF_PTR(line)->clear();
    IF_PTR(nl)->clear();

    int start = *i;
    while ( *i < (int)str.length() )
    {
        winux::String::value_type ch = str[*i];
        switch ( ch )
        {
        case '\n':
        case '\r':
            {
                if ( *i - start > 0 )
                    ASSIGN_PTR(line) = str.substr( start, *i - start ); // 不包括 \r \n
                switch ( ch )
                {
                case '\n':
                    ASSIGN_PTR(nl) += ch;
                    ++ *i; // skip '\n'
                    break;
                case '\r':
                    ASSIGN_PTR(nl) += ch;
                    ++ *i; // skip '\r'
                    if ( *i < (int)str.length() && ( ch = str[*i] ) == '\n' )
                    {
                        ASSIGN_PTR(nl) += ch;
                        ++ *i; // skip '\n'
                    }
                    break;
                }
                goto RETURN;
            }
            break;
        default:
            ++ *i;
            break;
        }
    }

    if ( *i == (int)str.length() )
    {
        ASSIGN_PTR(line) = str.substr(start);
    }
RETURN:
    return true;
}

// class OutStringStreamWrapper ------------------------------------------------------------------------
OutStringStreamWrapper::OutStringStreamWrapper( String * str, bool isAppend ) : _str(str), _sout(NULL), _isAppend(isAppend)
{
    _sout = new std::ostringstream();
}

OutStringStreamWrapper::OutStringStreamWrapper( OutStringStreamWrapper const & other ) : _str(NULL), _sout(NULL)
{
    this->operator = ( const_cast<OutStringStreamWrapper &>(other) );
}

OutStringStreamWrapper::~OutStringStreamWrapper()
{
    if ( _str )
    {
        if ( _isAppend )
        {
            *_str += _sout->str();
        }
        else
        {
            *_str = _sout->str();
        }
    }
    if ( _sout ) delete _sout;
}

OutStringStreamWrapper & OutStringStreamWrapper::operator = ( OutStringStreamWrapper & other )
{
    if ( this != &other )
    {
        _str = other._str;
        _sout = other._sout;
        _isAppend = other._isAppend;

        other._str = NULL;
        other._sout = NULL;
        other._isAppend = false;
    }
    return *this;
}

// -----------------------------------------------------------------------------------------
/* KMP字符串匹配算法 next值 */
template < typename _IndexType >
inline static std::vector< _IndexType > _Templ_KmpCalcNext( char const * substr, _IndexType sublen )
{
    std::vector< _IndexType > next( sublen + 1 );
    _IndexType j = 0, k = -1;
    next[0] = -1;
    while ( j < sublen )
    {
        if ( k == -1 || substr[j] == substr[k] )
        {
            j++;
            k++;
            if ( substr[j] != substr[k] )
                next[j] = k;
            else
                next[j] = next[k];
        }
        else
        {
            k = next[k];
        }
    }
    return next;
}

WINUX_FUNC_IMPL(std::vector<int> &) KmpCalcNext( char const * substr, int sublen, std::vector<int> * next )
{
    next->resize( sublen + 1 );
    int j = 0, k = -1;
    (*next)[0] = -1;
    while ( j < sublen )
    {
        if ( k == -1 || substr[j] == substr[k] )
        {
            j++;
            k++;
            if ( substr[j] != substr[k] )
                (*next)[j] = k;
            else
                (*next)[j] = (*next)[k];
        }
        else
        {
            k = (*next)[k];
        }
    }
    return *next;
}

/* KMP匹配 */
WINUX_FUNC_IMPL(int) KmpMatchEx( char const * str, int len, char const * substr, int sublen, int pos, std::vector<int> const & next )
{
    int i, j;
    i = pos;
    j = 0;
    while( i < len && j < sublen )
    {
        if( j == -1 || str[i] == substr[j] )
        {
            i++;
            j++;
        }
        else
        {
            j = next[j];
        }
    }
    return j == sublen ? i - sublen : -1;
}

WINUX_FUNC_IMPL(int) KmpMatch( char const * str, int len, char const * substr, int sublen, int pos )
{
    std::vector<int> next;
    return KmpMatchEx( str, len, substr, sublen, pos, KmpCalcNext( substr, sublen, &next ) );
}

// class MultiMatch -----------------------------------------------------------------------------
MultiMatch::MultiMatch( String matches[], uint m, String replaces[], uint n ) : _replaceFunc(NULL), _extra(NULL)
{
    init( StringArray( matches, matches + m ), StringArray( replaces, replaces + n ) );
}

MultiMatch::MultiMatch( String matches[], uint count, ReplaceFuncType replaceFunc, void * extra ) : _replaceFunc(NULL), _extra(NULL)
{
    init( StringArray( matches, matches + count ), replaceFunc, extra );
}

MultiMatch::MultiMatch( StringArray const & matches, StringArray const & replaces ) : _replaceFunc(NULL), _extra(NULL)
{
    init( matches, replaces );
}

MultiMatch::MultiMatch( StringArray const & matches, ReplaceFuncType replaceFunc, void * extra ) : _replaceFunc(NULL), _extra(NULL)
{
    init( matches, replaceFunc, extra );
}

MultiMatch::MultiMatch() : _replaceFunc(NULL), _extra(NULL)
{
}

void MultiMatch::init( StringArray const & matches, StringArray const & replaces )
{
    _matchItems = matches;
    _replaceItems = replaces;
    for ( StringArray::const_iterator it = _matchItems.begin(); it != _matchItems.end(); it++ )
    {
        _nextVals.push_back( _Templ_KmpCalcNext<short>( it->c_str(), it->length() ) );
    }
    _replaceFunc = NULL;
    _extra = NULL;
}

void MultiMatch::init( StringArray const & matches, ReplaceFuncType replaceFunc, void * extra )
{
    _matchItems = matches;

    for ( StringArray::const_iterator it = _matchItems.begin(); it != _matchItems.end(); it++ )
    {
        _nextVals.push_back( _Templ_KmpCalcNext<short>( it->c_str(), it->length() ) );
    }
    _replaceFunc = replaceFunc;
    _extra = extra;
}

int MultiMatch::addMatchReplacePair( String const & match, String const & replace )
{
    _matchItems.push_back(match);
    _nextVals.push_back( _Templ_KmpCalcNext<short>( match.c_str(), match.length() ) );
    _replaceItems.push_back(replace);
    return (int)_nextVals.size();
}

int MultiMatch::addMatch( String const & match )
{
    _matchItems.push_back(match);
    _nextVals.push_back( _Templ_KmpCalcNext<short>( match.c_str(), match.length() ) );
    return (int)_nextVals.size();
}

MultiMatch::ReplaceFuncType MultiMatch::setReplaceFunc( ReplaceFuncType newReplaceFunc, void * extra )
{
    ReplaceFuncType oldFunc = _replaceFunc;
    _replaceFunc = newReplaceFunc;
    _extra = extra;
    return oldFunc;
}

MultiMatch::MatchResult MultiMatch::search( String const & str, int offset ) const
{
    MatchResult res = { -1, -1 };
    int count = _matchItems.size();
    std::vector<MatchState> states(count);
    tchar const * mainStr = str.c_str() + offset;
    int i; // 主字符串当前字符位置
    bool nomove = false; // 不移动i
    memset( &states[0], 0, count * sizeof(MatchState) );
    i = 0;
    while ( mainStr[i] )
    {
        for ( int curr = 0; curr < count; curr++ ) // 各个匹配项进行匹配
        {
            if ( i < states[curr].markpos )
            {
            }
            else
            {
                String const & currItem = _matchItems[curr];
                int currItemLen = (int)currItem.length();
                if ( states[curr].j < currItemLen )
                {
                    if ( states[curr].j == -1 || mainStr[i] == currItem[states[curr].j] )
                    {
                        states[curr].j++;
                        states[curr].markpos = i + 1;
                        if ( states[curr].j == currItemLen ) // 表示当前项已经匹配成功
                        {
                            res.item = curr;
                            res.pos = i + 1 - currItemLen + offset;
                            goto RETURN;
                        }
                    }
                    else
                    {
                        states[curr].j = _nextVals[curr][states[curr].j];
                        nomove = true;
                    }
                }
                else // 表示已经匹配成功
                {
                    res.item = curr;
                    res.pos = i - currItemLen + offset;
                    goto RETURN;
                }
            }
        }
        if ( nomove )
        {
            nomove = false;
        }
        else
        {
            i++;
        }
    }
RETURN:
    return res;

}

/*MultiMatch::MatchResult MultiMatch::greedSearch( String const & str, int offset ) const
{
    MatchResult res = { -1, -1 };
    int count = _matchItems.size();
    String strRegex = StrJoin( "|", _matchItems );
    boost::regex r(strRegex);
    MatchResult res = { -1, -1 };
    int count = _matchItems.size();
    std::vector<MatchState> states(count);
    char const * mainStr = str.c_str() + offset;
    int i = 0; // 主字符串当前字符位置
    bool nomove = false; // 不移动i
    memset( &states[0], 0, count * sizeof(MatchState) );
    bool hasMatch = false;//有匹配项
    int matched;
    while ( mainStr[i] )
    {
        bool matchOneChar = false;//有一个字符匹配
        matched = 0;
        for ( int curr = 0; curr < count; curr++ ) // 各个匹配项进行匹配
        {

            if ( i < states[curr].markpos )
            {
            }
            else
            {
                String const & currItem = _matchItems[curr];
                int currItemLen = (int)currItem.length();
                if ( states[curr].j < currItemLen )
                {
                    if ( states[curr].j == -1 || mainStr[i] == currItem[states[curr].j] )
                    {
                        states[curr].j++;
                        states[curr].markpos = i + 1;
                        matchOneChar = true;
                        if ( states[curr].j == currItemLen ) // 表示当前项已经匹配成功
                        {
                            MatchResult mr;
                            mr.item = curr;
                            mr.pos = i + 1 - currItemLen + offset;
                            hasMatch = true;
                            res = mr;
                            matched++;
                            continue;
                        }
                    }
                    else
                    {
                        states[curr].j = _nextVals[curr][states[curr].j];
                        nomove = true;
                    }
                }
                else
                {

                }
            }
        }

        if ( matched == 1 )
            nomove = false;

        if ( hasMatch && ( !matchOneChar ) && matched == 0 )//有匹配项时，并且没有字符匹配时，检查匹配数组是否为空，然后跳出
            break;

        if ( nomove )
            nomove = false;
        else
            i++;
    }

    return res;

}//*/

MultiMatch::MatchResult MultiMatch::commonSearch( String const & str, int offset ) const
{
    MatchResult r = { -1, -1 };
    int i;
    for ( i = offset; i < (int)str.length(); ++i )
    {
        int matchItemCount = _matchItems.size();
        int curr;
        for ( curr = 0; curr < matchItemCount; ++curr )
        {
            String const & matchItem = _matchItems[curr];
            if ( str.length() - i < matchItem.length() )
                continue;
            int j;
            bool isMatch = true;
            for ( j = 0; j < (int)matchItem.length(); ++j )
            {
                if ( str[j + i] != matchItem[j] )
                {
                    isMatch = false;
                    break;
                }
            }
            if ( isMatch )
            {
                r.item = curr;
                r.pos = i;
                return r;
            }
        }
    }
    return r;
}

String MultiMatch::replace( String const & str, MatchResult ( MultiMatch:: * fnSearch )( String const & str, int offset ) const ) const
{
    String s = "";
    int offset = 0;
    //int len = (int)str.length();
    MatchResult r;

    //fnSearch = &MultiMatch::search;
    r = (this->*fnSearch)( str.c_str() + offset, 0 );
    while ( r.pos != -1 )
    {
        s += String( str.c_str() + offset, str.c_str() + offset + r.pos );
        if ( _replaceFunc )
            s += _replaceFunc( this, r.item, _extra );
        else
            s += _replaceItems[r.item];
        offset += r.pos + _matchItems[r.item].length();
        r = (this->*fnSearch)( str.c_str() + offset, 0 );
    }
    s += str.c_str() + offset;
    return s;
}

String const & MultiMatch::getMatchItem( int item ) const
{
    return _matchItems[item];
}

String MultiMatch::getReplaceItem( int item ) const
{
    if ( _replaceFunc )
        return _replaceFunc( this, item, _extra );
    else
        return _replaceItems[item];
}

void MultiMatch::setReplaceItem( int item, String const & replace )
{
    _replaceItems[item] = replace;
}

void MultiMatch::clear()
{
    this->_matchItems.clear();
    this->_nextVals.clear();
    this->_replaceItems.clear();
    this->_extra = NULL;
    this->_replaceFunc = NULL;
}

// ---------------------------------------------------------------------------
// 本地字符串和unicode字符串互转支持
// ---------------------------------------------------------------------------
AnsiString SetLocale::_clsLc = "";

SetLocale::SetLocale( char const * lc )
{
    _loc = lc ? lc : _clsLc;
    _prevLoc = setlocale( LC_ALL, NULL );
    setlocale( LC_ALL, _loc.c_str() );
}

SetLocale::~SetLocale()
{
    setlocale( LC_ALL, _prevLoc.c_str() );
}

#if defined(__GNUC__) && !defined(WIN32)
size_t mbslen( char const * str, int size )
{
    SetLocale __setLoc;
    size_t cch = 0;
    char const * p = str;
    int oneCharSize = -1;
    while ( p - str < size && ( oneCharSize = mblen( p, MB_CUR_MAX ) ) > 0 )
    {
        p += oneCharSize;
        cch++;
    }
    return cch;
}
#endif

WINUX_FUNC_IMPL(uint) LocalCharsCount( LocalString const & local )
{
    SetLocale __setLoc;
    uint cnt = mbstowcs( NULL, local.c_str(), local.length() );
    if ( cnt != (uint)-1 ) return cnt;
#if defined(_MSC_VER) || defined(WIN32)
    return _mbslen( (unsigned char *)local.c_str() );
#else
    return mbslen( local.c_str(), local.length() );
#endif
}

WINUX_FUNC_IMPL(uint) UnicodeMinLength( UnicodeString const & unicode )
{
    SetLocale __setLoc;
    uint len = wcstombs( NULL, unicode.c_str(), 0 );
    if ( len != (uint)-1 ) return len;
    len = 0;
    UnicodeString::const_iterator it;
    for ( it = unicode.begin(); it != unicode.end(); ++it )
        len += ((unsigned int)*it) > 0xFFU ? 2 : 1;
    return len;
}

WINUX_FUNC_IMPL(LocalString) UnicodeToLocal( UnicodeString const & unicode )
{
    SetLocale __setLoc;
    LocalString r;
    r.resize( UnicodeMinLength(unicode) + 1 );
    wcstombs( &r[0], unicode.c_str(), r.size() );
    return r.c_str();
}

WINUX_FUNC_IMPL(UnicodeString) LocalToUnicode( LocalString const & local )
{
    SetLocale __setLoc;
    UnicodeString r;
    r.resize( LocalCharsCount(local) + 1 );
    mbstowcs( &r[0], local.c_str(), local.length() );
    return r.c_str();
}

WINUX_FUNC_IMPL(String) LocalToString( LocalString const & local )
{
#ifdef UNICODE
    return LocalToUnicode(local);
#else
    return local;
#endif
}

WINUX_FUNC_IMPL(String) UnicodeToString( UnicodeString const & unicode )
{
#ifdef UNICODE
    return unicode;
#else
    return UnicodeToLocal(unicode);
#endif
}

WINUX_FUNC_IMPL(LocalString) StringToLocal( String const & str )
{
#ifdef UNICODE
    return UnicodeToLocal(str);
#else
    return str;
#endif
}

WINUX_FUNC_IMPL(UnicodeString) StringToUnicode( String const & str )
{
#ifdef UNICODE
    return str;
#else
    return LocalToUnicode(str);
#endif
}

String FormatExV( uint cch, tchar const * fmt, va_list args )
{
    String str;
    str.resize( cch + 1 );
    _vsntprintf( &str[0], cch, fmt, args );
    return str.c_str();
}

String FormatEx( uint cch, tchar const * fmt, ... )
{
    va_list args;
    va_start( args, fmt );
    return FormatExV( cch, fmt, args );
}

String Format( tchar const * fmt, ... )
{
    va_list args;
    va_start( args, fmt );

#if defined(_MSC_VER) || defined(WIN32)
    int c = _vsctprintf( fmt, args );
    return FormatExV( c, fmt, args );
#else
    char * buf = NULL;
    vasprintf( &buf, fmt, args );
    String s = buf;
    free(buf);
    return s;
#endif
}

// class SZInput ------------------------------------------------------------------------------------
SZInput & SZInput::operator = ( char const * pstr )
{
    pstr = pstr ? pstr : "";
    switch ( _type )
    {
    case szCharInput:
        {
            size_t uTextLen = strlen(pstr);
            if ( _count - 1 < uTextLen )
            {
                memcpy( _psz, pstr, sizeof(char) * ( _count - 1 ) );
                _psz[_count - 1] = 0;
            }
            else
            {
                memcpy( _psz, pstr, sizeof(char) * uTextLen );
                _psz[uTextLen] = 0;
            }
        }
        break;
    case szWCharInput:
        {
            UnicodeString str = LocalToUnicode(pstr);
            size_t uTextLen = str.length();
            if ( _count - 1 < uTextLen )
            {
                memcpy( _pwsz, str.c_str(), sizeof(wchar_t) * ( _count - 1 ) );
                _pwsz[_count - 1] = 0;
            }
            else
            {
                memcpy( _pwsz, str.c_str(), sizeof(wchar_t) * uTextLen );
                _pwsz[uTextLen] = 0;
            }
        }
        break;
    }
    return *this;
}

SZInput & SZInput::operator = ( wchar_t const * pwstr )
{
    pwstr = pwstr ? pwstr : L"";
    switch ( _type )
    {
    case szCharInput:
        {
            LocalString str = UnicodeToLocal(pwstr);
            size_t uTextLen = str.length();
            if ( _count - 1 < uTextLen )
            {
                memcpy( _psz, str.c_str(), sizeof(char) * ( _count - 1 ) );
                _psz[_count - 1] = 0;
            }
            else
            {
                memcpy( _psz, str.c_str(), sizeof(char) * uTextLen );
                _psz[uTextLen] = 0;
            }
        }
        break;
    case szWCharInput:
        {
            size_t uTextLen = wcslen(pwstr);
            if ( _count - 1 < uTextLen )
            {
                memcpy( _pwsz, pwstr, sizeof(wchar_t) * ( _count - 1 ) );
                _pwsz[_count - 1] = 0;
            }
            else
            {
                memcpy( _pwsz, pwstr, sizeof(wchar_t) * uTextLen );
                _pwsz[uTextLen] = 0;
            }
        }
        break;
    }
    return *this;
}

// struct Conv_Data -----------------------------------------------------------------------
struct Conv_Data
{
#if defined(__GNUC__) || defined(HAVE_ICONV)
    //转换句柄
    iconv_t _cd;
#else
    uint _fromCP;
    uint _toCP;
#endif

};

// class Conv -----------------------------------------------------------------------------
#if !defined(__GNUC__) && !defined(HAVE_ICONV)
static struct __ConvLangCodePage
{
    std::map< String, uint > _convLangCP;
    __ConvLangCodePage()
    {
        _convLangCP[""] = CP_ACP;
        _convLangCP["ASCII"] = CP_ACP;
        _convLangCP["CHAR"] = CP_ACP;
        _convLangCP["UTF-7"] = CP_UTF7;
        _convLangCP["UTF-8"] = CP_UTF8;
        _convLangCP["WCHAR_T"] = 1200;
        _convLangCP["UCS-2LE"] = 1200;
        _convLangCP["UCS-2"] = 1201;
        _convLangCP["UCS-2BE"] = 1201;
        _convLangCP["GBK"] = 936;
        _convLangCP["SHIFT_JIS"] = 932;
        _convLangCP["BIG5"] = 950;
    }
    // 根据语言串获取代码页编码
    uint operator [] ( String cp ) const
    {
        if ( !cp.empty() ) StrMakeUpper(&cp);

        if ( cp.length() > 1 && cp.substr( 0, 2 ) == "CP" )
        {
            return atoi( cp.c_str() + 2 );
        }
        if ( isset( _convLangCP, cp ) )
        {
            return _convLangCP.at(cp);
        }
        return 0;
    }
    // 是否为宽字符编码
    bool isWideChar( uint cp ) const { return cp == 1200 || cp == 1201; }
    // 判断该代码页在执行WideCharToMultiByte()和MultiByteToWideChar()时dwFlags是否必须为0
    bool isFlagMustZero( uint cp ) const
    {
        return
            cp == 50220 ||
            cp == 50221 ||
            cp == 50222 ||
            cp == 50225 ||
            cp == 50227 ||
            cp == 50229 ||
            cp == 52936 ||
            cp == 54936 ||
            cp >= 57002 && cp <= 57011 ||
            cp == 65000 ||
            cp == 65001;
    }

} __LangCP;

// UCS-2 大小端次序转换
static void __Ucs2LeBe( wchar * inOutBuf, uint cch )
{
    uint i;
    for ( i = 0; i < cch; ++i )
    {
        union
        {
            wchar ch;
            struct
            {
                byte low;
                byte hig;
            };
        } a, b;
        a.ch = inOutBuf[i];
        b.low = a.hig;
        b.hig = a.low;
        inOutBuf[i] = b.ch;
    }
}

// 多字节到宽字符,*str2自动分配内存，用户负责free(),返回写入*str2缓冲区的字节数
static int __MbsToWcs( uint cp1, char const * str1, uint size1, uint cp2, wchar * * str2 )
{
    // 你得先获得缓冲区大小,字符数
    int cch = MultiByteToWideChar(
        cp1,
        0,
        str1,
        size1,
        NULL,
        0
    );
    int size2 = sizeof(wchar) * ( cch + 1 ) ;
    *str2 = (wchar *)malloc(size2); // 分配内存
    memset( *str2, 0, size2 );
    // 这才进行转换
    size2 = sizeof(wchar) * MultiByteToWideChar(
        cp1,
        0,
        str1,
        size1,
        *str2,
        cch + 1
    );
    // 如果是UCS-2BE
    if ( cp2 == 1201 )
    {
        __Ucs2LeBe( *str2, cch );
    }
    return size2;
}

// 宽字符到多字节,注意:size1必须是字节数，即,宽字符数*sizeof(wchar),*str2自动分配内存，用户负责free(),返回写入*str2缓冲区的字节数
static int __WcsToMbs( uint cp1, wchar const * str1, uint size1, uint cp2, char * * str2 )
{
    UnicodeString strTmp(str1);
    if ( cp1 == 1201 )
    {
        if ( !strTmp.empty() ) __Ucs2LeBe( &strTmp[0], strTmp.length() );
        str1 = strTmp.c_str();
    }
    //LOG( Format( "cp2=%d,str1=%s,size1=%d", cp2, UnicodeToLocal(str1).c_str(), size1 ) );
    int length = WideCharToMultiByte(
        cp2,
        0 /*| ( cp2 == CP_UTF8 ? WC_ERR_INVALID_CHARS : 0 )*/,
        str1,
        size1 / sizeof(wchar),
        NULL,
        0,
        NULL,
        NULL
    );
    int size2 = sizeof(char) * ( length + 1 );
    *str2 = (char *)malloc(size2);
    memset( *str2, 0, size2 );

    //LOG( Format("size2=%d,length=%d",size2,length) );
    size2 = WideCharToMultiByte(
        cp2,
        0 /*| ( cp2 == CP_UTF8 ? WC_ERR_INVALID_CHARS : 0 )*/,
        str1,
        size1 / sizeof(wchar),
        *str2,
        length + 1,
        NULL,
        NULL
    );
    if ( size2 == 0 )
    {
        LOG( Format("GetLastError() %X", GetLastError() ) );
    }

    return size2;
}

// 多字节字符到多字节字符,*str2自动分配内存，用户负责free(),返回写入*str2缓冲区的字节数
static int __MbsToMbs( uint cp1, char const * str1, uint size1, uint cp2, char * * str2 )
{
    // 首先要转到Unicode
    wchar * tmpUcs;
    //LOG( Format("mbstombs() cp1:%d to cp2:%d", cp1, cp2 ) );
    int tmpUcsSize = __MbsToWcs( cp1, str1, size1, 0, &tmpUcs );
    //LOG( Format("mbstowcs() OK, ucs bytes:%d", tmpUcsSize ) );
    int size2 = __WcsToMbs( 0, tmpUcs, tmpUcsSize, cp2, str2 );
    //LOG( Format("wcstombs() OK, mbs bytes:%u", size2 ) );
    free(tmpUcs);

    return size2;
}

// 字符串编码转换,返回写入*str2缓冲区的数量
static int __StrCodeConvert( uint cp1, char const * str1, uint size1, uint cp2, char ** str2 )
{
    int size2 = 0;
    // 4种方式,1 m to w, 2 w to m, 3 m to m, 4 w to w
    if ( __LangCP.isWideChar(cp1) ) // cp1 is w
    {
        if ( __LangCP.isWideChar(cp2) ) // cp2 is w
        {
            // w to w, directly copy
            int cch1 = size1 / sizeof(wchar);
            size2 = ( cch1 + 1 ) * sizeof(wchar);
            *str2 = (char *)malloc(size2);
            memset( *str2, 0, size2 );
            memcpy( *str2, str1, size1 );
            if ( cp1 != cp2 )
            {
                __Ucs2LeBe( (wchar *)*str2, cch1 );
            }
            return cch1;
        }
        else // cp2 is m
        {
            // w to m
            return __WcsToMbs( cp1, (wchar *)str1, size1, cp2, str2 );
        }
    }
    else // cp1 is m
    {
        if ( __LangCP.isWideChar(cp2) ) // cp2 is w
        {
            return __MbsToWcs( cp1, str1, size1, cp2, (wchar **)str2 );
        }
        else // cp2 is m
        {
            return __MbsToMbs( cp1, str1, size1, cp2, str2 );
        }
    }
    return 0;
}

#endif

Conv::Conv( char const * fromCode, char const * toCode )
{
    _self.create(); //

#if defined(__GNUC__) || defined(HAVE_ICONV)
    _self->_cd = iconv_open( toCode, fromCode );
#else
    _self->_fromCP = __LangCP[fromCode];
    _self->_toCP = __LangCP[toCode];
#endif
}

Conv::~Conv()
{
#if defined(__GNUC__) || defined(HAVE_ICONV)
    iconv_close(_self->_cd);
#endif

    _self.destroy(); //
}

int Conv::convert( char const * srcBuf, size_t srcSize, char * * destBuf )
{
#if defined(__GNUC__) || defined(HAVE_ICONV)
    int r = 0, outBytes = 0, err = 0;
    size_t destSize = srcSize;
    *destBuf = NULL;
    do
    {
        char * srcP = (char *)srcBuf;
        size_t srcN = srcSize;
        destSize <<= 1;
        *destBuf = (char *)realloc( *destBuf, destSize );
        char * buf = *destBuf;
        size_t outBytesLeft = destSize;
        memset( *destBuf, 0, destSize );
        //返回不可逆转换的字符个数
        r = iconv( _self->_cd, &srcP, &srcN, &buf, &outBytesLeft );
        outBytes = destSize - outBytesLeft; // 求得输出的字节数
        err = errno;
    } while ( r == -1 && err == E2BIG );
    return outBytes;
#else
    return __StrCodeConvert( _self->_fromCP, srcBuf, srcSize, _self->_toCP, destBuf );
#endif
}

//ConvFrom<AnsiString> LocalFromUtf8("UTF-8");
WINUX_FUNC_IMPL(AnsiString) LocalFromUtf8( AnsiString const & str )
{
    ConvFrom<AnsiString> conv("UTF-8");
    return conv(str);
}
//ConvTo<AnsiString> LocalToUtf8("UTF-8");
WINUX_FUNC_IMPL(AnsiString) LocalToUtf8( AnsiString const & str )
{
    ConvTo<AnsiString> conv("UTF-8");
    return conv(str);
}


} // namespace winux
