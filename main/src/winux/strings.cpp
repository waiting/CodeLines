#include "system_detection.inl"

#if defined(CL_MINGW) // for mingw
    #ifdef __STRICT_ANSI__
    #undef __STRICT_ANSI__
    #endif
#endif

#include "utilities.hpp"
#include "strings.hpp"

#if defined(__GNUC__) || defined(HAVE_ICONV_H)
#include <iconv.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <math.h>
#include <iomanip>

#if defined(OS_WIN) // IS_WINDOWS
    #include <mbstring.h>
    #include <tchar.h>

    #ifdef UNICODE
    #define _vsntprintf _vsnwprintf
    #else
    #define _vsntprintf _vsnprintf
    #endif

#else
    #include <errno.h>
    #define _vsnprintf vsnprintf
    #define _vsnwprintf vswprintf

    #ifdef UNICODE
    #define _vsntprintf vswprintf
    #define _tcsstr wcsstr
    #else
    #define _vsntprintf vsnprintf
    #define _tcsstr strstr
    #endif

    #define swprintf_s swprintf
#endif

namespace winux
{

#include "is_x_funcs.inl"

// 字符是否在给定的列表中
template < typename _ChTy >
inline static bool InCharList( _ChTy ch, XString<_ChTy> const & charlist )
{
    return charlist.find(ch) != XString<_ChTy>::npos;
}

template < typename _ChTy >
inline static int Impl_StrSplit( XString<_ChTy> const & str, XString<_ChTy> const & delimList, XStringArray<_ChTy> * arr, bool alwaysRetOneElem )
{
    if ( !alwaysRetOneElem && str.empty() ) return 0;

    int count = 0;
    XString<_ChTy> tmp;

    for ( size_t i = 0; i < str.length(); i++ )
    {
        if ( InCharList( str[i], delimList ) )
        {
            arr->push_back(tmp);
            count++;
            tmp.clear();
        }
        else
        {
            tmp += str[i];
        }
    }
    arr->push_back(tmp);
    count++;
    return count;
}

WINUX_FUNC_IMPL(int) StrSplitA( AnsiString const & str, AnsiString const & delimList, AnsiStringArray * arr, bool alwaysRetOneElem )
{
    return Impl_StrSplit( str, delimList, arr, alwaysRetOneElem );
}

WINUX_FUNC_IMPL(int) StrSplitW( UnicodeString const & str, UnicodeString const & delimList, UnicodeStringArray * arr, bool alwaysRetOneElem )
{
    return Impl_StrSplit( str, delimList, arr, alwaysRetOneElem );
}

template < typename _ChTy >
inline static int Impl_StrSplit2( XString<_ChTy> const & str, XString<_ChTy> const & delim, XStringArray<_ChTy> * arr, bool alwaysRetOneElem )
{
    if ( !alwaysRetOneElem && str.empty() ) return 0;
    if ( delim.empty() )
    {
        arr->push_back(str);
        return 1;
    }

    int count = 0;
    size_t cur = 0;
    size_t found = cur;

    while ( ( found = str.find( delim, cur ) ) != XString<_ChTy>::npos )
    {
        arr->push_back( str.substr( cur, found - cur ) );
        count++;
        cur = found + delim.length();
    }
    arr->push_back( str.substr(cur) );
    count++;
    return count;
}

WINUX_FUNC_IMPL(int) StrSplit2A( AnsiString const & str, AnsiString const & delim, AnsiStringArray * arr, bool alwaysRetOneElem )
{
    return Impl_StrSplit2( str, delim, arr, alwaysRetOneElem );
}

WINUX_FUNC_IMPL(int) StrSplit2W( UnicodeString const & str, UnicodeString const & delim, UnicodeStringArray * arr, bool alwaysRetOneElem )
{
    return Impl_StrSplit2( str, delim, arr, alwaysRetOneElem );
}

template < typename _ChTy >
inline static XString<_ChTy> Impl_StrJoin( XString<_ChTy> const & delim, XStringArray<_ChTy> const & arr )
{
    XString<_ChTy> res;
    size_t count = arr.size();
    size_t i;
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

WINUX_FUNC_IMPL(AnsiString) StrJoinA( AnsiString const & delim, AnsiStringArray const & arr )
{
    return Impl_StrJoin( delim, arr );
}

WINUX_FUNC_IMPL(UnicodeString) StrJoinW( UnicodeString const & delim, UnicodeStringArray const & arr )
{
    return Impl_StrJoin( delim, arr );
}

template < typename _ChTy >
inline static XString<_ChTy> Impl_StrJoinEx( XString<_ChTy> const & delim, XStringArray<_ChTy> const & arr, int start, int elemCount )
{
    XString<_ChTy> res;
    int count = 0;
    if ( elemCount < 0 )
    {
        count = (int)arr.size() - start;
    }
    else
    {
        count = ( elemCount < (int)arr.size() - start ? elemCount : (int)arr.size() - start );
    }

    for ( int i = 0; i < count; i++ )
    {
        if ( i != 0 )
        {
            res += delim;
        }
        res += arr[i + start];
    }
    return res;
}

WINUX_FUNC_IMPL(AnsiString) StrJoinExA( AnsiString const & delim, AnsiStringArray const & arr, int start, int elemCount )
{
    return Impl_StrJoinEx( delim, arr, start, elemCount );
}

WINUX_FUNC_IMPL(UnicodeString) StrJoinExW( UnicodeString const & delim, UnicodeStringArray const & arr, int start, int elemCount )
{
    return Impl_StrJoinEx( delim, arr, start, elemCount );
}

template < typename _ChTy >
inline static XString<_ChTy> Impl_StrInsert( XString<_ChTy> const & str, int start, int end, XString<_ChTy> const & insert )
{
    XString<_ChTy> res;
    res += str.substr( 0, start );
    res += insert;
    if ( end < (int)str.length() )
        res += str.substr(end);
    return res;
}

WINUX_FUNC_IMPL(AnsiString) StrInsertA( AnsiString const & str, int start, int end, AnsiString const & insert )
{
    return Impl_StrInsert( str, start, end, insert );
}

WINUX_FUNC_IMPL(UnicodeString) StrInsertW( UnicodeString const & str, int start, int end, UnicodeString const & insert )
{
    return Impl_StrInsert( str, start, end, insert );
}

template < typename _ChTy >
inline static XString<_ChTy> & Impl_StrMakeReplace( XString<_ChTy> * str, XString<_ChTy> const & findText, XString<_ChTy> const & replaceText, size_t offset )
{
    size_t pos = offset;
    while ( ( pos = str->find( findText, pos ) ) != XString<_ChTy>::npos )
    {
        str->replace( pos, findText.length(), replaceText );
        pos += replaceText.length();
    }
    return *str;
}

WINUX_FUNC_IMPL(AnsiString &) StrMakeReplaceA( AnsiString * str, AnsiString const & findText, AnsiString const & replaceText, size_t offset )
{
    return Impl_StrMakeReplace( str, findText, replaceText, offset );
}

WINUX_FUNC_IMPL(UnicodeString &) StrMakeReplaceW( UnicodeString * str, UnicodeString const & findText, UnicodeString const & replaceText, size_t offset )
{
    return Impl_StrMakeReplace( str, findText, replaceText, offset );
}

template < typename _ChTy >
inline static XString<_ChTy> Impl_StrTrim( XString<_ChTy> const & str )
{
    _ChTy const * p1, * p2;
    p1 = str.c_str();
    p2 = str.c_str() + str.length();
    while ( p1 < p2 && ( *p1 == Literal<_ChTy>::spaceChar || *p1 == Literal<_ChTy>::crChar || *p1 == Literal<_ChTy>::lfChar || *p1 == Literal<_ChTy>::htChar || *p1 == Literal<_ChTy>::vtChar ) )
    {
        p1++;
    }
    while ( p2 > p1 && ( p2[-1] == Literal<_ChTy>::spaceChar || p2[-1] == Literal<_ChTy>::crChar || p2[-1] == Literal<_ChTy>::lfChar || p2[-1] == Literal<_ChTy>::htChar || p2[-1] == Literal<_ChTy>::vtChar ) )
    {
        p2--;
    }

    return XString<_ChTy>( p1, p2 );
}

WINUX_FUNC_IMPL(AnsiString) StrTrimA( AnsiString const & str )
{
    return Impl_StrTrim(str);
}

WINUX_FUNC_IMPL(UnicodeString) StrTrimW( UnicodeString const & str )
{
    return Impl_StrTrim(str);
}

template < typename _ChTy >
inline static XString<_ChTy> & Impl_StrMakeUpper( XString<_ChTy> * str )
{
    std::transform( str->begin(), str->end(), str->begin(), toupper );
    return *str;
}

template < typename _ChTy >
inline static XString<_ChTy> & Impl_StrMakeLower( XString<_ChTy> * str )
{
    std::transform( str->begin(), str->end(), str->begin(), tolower );
    return *str;
}

WINUX_FUNC_IMPL(AnsiString &) StrMakeUpperA( AnsiString * str )
{
    return Impl_StrMakeUpper(str);
}

WINUX_FUNC_IMPL(AnsiString) StrUpperA( AnsiString str )
{
    Impl_StrMakeUpper(&str);
    return std::move(str);
}

WINUX_FUNC_IMPL(UnicodeString &) StrMakeUpperW( UnicodeString * str )
{
    return Impl_StrMakeUpper(str);
}

WINUX_FUNC_IMPL(UnicodeString) StrUpperW( UnicodeString str )
{
    Impl_StrMakeUpper(&str);
    return std::move(str);
}

WINUX_FUNC_IMPL(AnsiString &) StrMakeLowerA( AnsiString * str )
{
    return Impl_StrMakeLower(str);
}

WINUX_FUNC_IMPL(AnsiString) StrLowerA( AnsiString str )
{
    Impl_StrMakeLower(&str);
    return std::move(str);
}

WINUX_FUNC_IMPL(UnicodeString &) StrMakeLowerW( UnicodeString * str )
{
    return Impl_StrMakeLower(str);
}

WINUX_FUNC_IMPL(UnicodeString) StrLowerW( UnicodeString str )
{
    Impl_StrMakeLower(&str);
    return std::move(str);
}

template < typename _ChTy >
inline static XString<_ChTy> Impl_StrMultiple( XString<_ChTy> const & str, int multiple )
{
    XString<_ChTy> r;
    for ( int i = 0; i < multiple; i++ ) r += str;
    return r;
}

WINUX_FUNC_IMPL(AnsiString) StrMultipleA( AnsiString const & str, int multiple )
{
    return Impl_StrMultiple( str, multiple );
}

WINUX_FUNC_IMPL(UnicodeString) StrMultipleW( UnicodeString const & str, int multiple )
{
    return Impl_StrMultiple( str, multiple );
}

template < typename _ChTy >
inline static XString<_ChTy> Impl_StrSubtract( XString<_ChTy> str1, XString<_ChTy> const & str2 )
{
    Impl_StrMakeReplace<_ChTy>( &str1, str2, Literal<_ChTy>::emptyStr, 0 );
    return std::move(str1);
}

WINUX_FUNC_IMPL(AnsiString) StrSubtractA( AnsiString str1, AnsiString const & str2 )
{
    return Impl_StrSubtract( str1, str2 );
}

WINUX_FUNC_IMPL(UnicodeString) StrSubtractW( UnicodeString str1, UnicodeString const & str2 )
{
    return Impl_StrSubtract( str1, str2 );
}

#if defined(__GNUC__) /*&& !defined(WIN32)*/
#define _UI64_MAX 0xffffffffffffffffull
#define _I64_MAX 9223372036854775807ll
#define _I64_MIN (-9223372036854775807ll - 1ll)
#endif

template < typename _ChTy >
inline static uint64 Impl_StrToXq( _ChTy const * nptr, _ChTy const ** endptr, int ibase, int flags )
{
    _ChTy const * p;
    _ChTy c;
    uint64 number;
    uint digval;
    uint64 maxval;

    p = nptr; /* p is our scanning pointer */
    number = 0; /* start with zero */

    c = *p++; /* read char */

    while ( isspace((int)(unsigned char)c) )
        c = *p++; /* skip whitespace */

    if ( c == Literal<_ChTy>::negativeChar )
    {
        flags |= stqNegative; /* remember minus sign */
        c = *p++;
    }
    else if ( c == Literal<_ChTy>::positiveChar )
        c = *p++; /* skip sign */

    if ( ibase < 0 || ibase == 1 || ibase > 36 )
    {
        /* bad base! */
        if ( endptr )
        {
            /* store beginning of string in endptr */
            *endptr = nptr;
        }
        return 0L; /* return 0 */
    }
    else if ( ibase == 0 )
    {
        /* determine base free-lance, based on first two chars of string */
        if ( c != Literal<_ChTy>::zeroChar )
            ibase = 10;
        else if ( ( *p | 0x20 ) == Literal<_ChTy>::xChar )
            ibase = 16;
        else
            ibase = 8;
    }

    if ( ibase == 16 )
    {
        /* we might have 0x in front of number; remove if there */
        if ( c == Literal<_ChTy>::zeroChar && ( *p | 0x20 ) == Literal<_ChTy>::xChar )
        {
            ++p;
            c = *p++; /* advance past prefix */
        }
    }

    /* if our number exceeds this, we will overflow on multiply */
    maxval = _UI64_MAX / ibase;

    for ( ; ; ) /* exit in middle of loop */
    {
        /* convert c to value */
        if ( isdigit((int)(unsigned char)c) )
            digval = c - Literal<_ChTy>::zeroChar;
        else if ( isalpha((int)(unsigned char)c) )
            digval = toupper(c) - Literal<_ChTy>::AChar + 10;
        else
            break;

        if ( digval >= (unsigned)ibase )
            break; /* exit loop if bad digit found */

        /* record the fact we have read one digit */
        flags |= stqReadDigit;

        /* we now need to compute number = number * base + digval,
           but we need to know if overflow occured.  This requires
           a tricky pre-check. */

        if ( number < maxval || ( number == maxval && (uint64)digval <= _UI64_MAX % ibase ) )
        {
            /* we won't overflow, go ahead and multiply */
            number = number * ibase + digval;
        }
        else
        {
            /* we would have overflowed -- set the overflow flag */
            flags |= stqOverflow;
        }

        c = *p++; /* read next digit */
    }

    --p; /* point to place that stopped scan */

    if ( !( flags & stqReadDigit ) )
    {
        /* no number there; return 0 and point to beginning of string */
        if (endptr)
        {
            /* store beginning of string in endptr later on */
            p = nptr;
        }
        number = 0L; /* return 0 */
    }
    else if (
        ( flags & stqOverflow ) ||
        (
            !( flags & stqUnsigned ) &&
            (
                ( ( flags & stqNegative ) && ( number > -_I64_MIN ) ) || ( !( flags & stqNegative ) && ( number > _I64_MAX ) ) 
            ) 
        ) 
    )
    {
        /* overflow or signed overflow occurred */
        errno = ERANGE;
        if ( flags & stqUnsigned )
            number = _UI64_MAX;
        else if ( flags & stqNegative )
            number = _I64_MIN;
        else
            number = _I64_MAX;
    }

    if ( endptr != NULL )
    {
        /* store pointer to char that stopped the scan */
        *endptr = p;
    }

    if ( flags & stqNegative )
    {
        /* negate result if there was a neg sign */
        number = (uint64)(-(int64)number);
    }

    return number; /* done. */
}

WINUX_FUNC_IMPL(uint64) StrToXqA( char const * nptr, char const ** endptr, int ibase, int flags )
{
    return Impl_StrToXq( nptr, endptr, ibase, flags );
}

WINUX_FUNC_IMPL(uint64) StrToXqW( wchar const * nptr, wchar const ** endptr, int ibase, int flags )
{
    return Impl_StrToXq( nptr, endptr, ibase, flags );
}

WINUX_FUNC_IMPL(int64) StrToInt64A( AnsiString const & numStr, int ibase )
{
    return (int64)Impl_StrToXq<AnsiString::value_type>( numStr.c_str(), nullptr, ibase, 0 );
}

WINUX_FUNC_IMPL(int64) StrToInt64W( UnicodeString const & numStr, int ibase )
{
    return (int64)Impl_StrToXq<UnicodeString::value_type>( numStr.c_str(), nullptr, ibase, 0 );
}

WINUX_FUNC_IMPL(uint64) StrToUint64A( AnsiString const & numStr, int ibase )
{
    return Impl_StrToXq<AnsiString::value_type>( numStr.c_str(), nullptr, ibase, stqUnsigned );
}

WINUX_FUNC_IMPL(uint64) StrToUint64W( UnicodeString const & numStr, int ibase )
{
    return Impl_StrToXq<UnicodeString::value_type>( numStr.c_str(), nullptr, ibase, stqUnsigned );
}

// add/strip slashes internal functions ------------------------------------------------------------------
template < typename _ChTy >
inline static bool IsSpecial( _ChTy ch )
{
    return
        ch == Literal<_ChTy>::aChar ||
        ch == Literal<_ChTy>::bChar ||
        ch == Literal<_ChTy>::tChar ||
        ch == Literal<_ChTy>::nChar ||
        ch == Literal<_ChTy>::vChar ||
        ch == Literal<_ChTy>::fChar ||
        ch == Literal<_ChTy>::rChar
    ;
}

template < typename _ChTy >
inline static _ChTy NumberStringToChar( _ChTy const * number, int base )
{
    _ChTy const * endptr;
    return (_ChTy)Impl_StrToXq<_ChTy>( number, &endptr, base, 0 );
}

template < typename _ChTy >
inline static _ChTy SpecialToChar( _ChTy special )
{
    switch ( special )
    {
    case Literal<_ChTy>::aChar: // 响铃(BEL) 07H/7
        special = Literal<_ChTy>::belChar;
        break;
    case Literal<_ChTy>::bChar: // 退格符(BS) 08H/8
        special = Literal<_ChTy>::bsChar;
        break;
    case Literal<_ChTy>::tChar: // 水平制表符(HT) 09H/9
        special = Literal<_ChTy>::htChar;
        break;
    case Literal<_ChTy>::nChar: // 换行符(LF) 0AH/10
        special = Literal<_ChTy>::lfChar;
        break;
    case Literal<_ChTy>::vChar: // 垂直制表(VT) 0BH/11
        special = Literal<_ChTy>::vtChar;
        break;
    case Literal<_ChTy>::fChar: // 换页符(FF) 0CH/12
        special = Literal<_ChTy>::ffChar;
        break;
    case Literal<_ChTy>::rChar: // 回车符(CR) 0DH/13
        special = Literal<_ChTy>::crChar;
        break;
    }
    return special;
}

template < size_t _Size > struct UT;
template <> struct UT<1> { using type = uint8; };
template <> struct UT<2> { using type = uint16; };
template <> struct UT<4> { using type = uint32; };
template <> struct UT<8> { using type = uint64; };

template < typename _ChTy >
inline static XString<_ChTy> CharToHexStr( _ChTy ch )
{
    std::basic_ostringstream<_ChTy> sout;
    sout
        << std::setw(sizeof(_ChTy) * 2)
        << std::setfill(Literal<_ChTy>::zeroChar)
        << std::hex
        << (uint32)(typename UT< sizeof(_ChTy) >::type)ch
    ;
    return sout.str();
}

template < typename _ChTy >
inline static XString<_ChTy> Impl_AddSlashes( XString<_ChTy> const & str, XString<_ChTy> const & charlist )
{
    using MyConstIterator = typename XString<_ChTy>::const_iterator;

    XString<_ChTy> slashes;
    for ( MyConstIterator it = str.begin(); it != str.end(); it++ )
    {
        _ChTy ch = *it;
        if ( charlist.find(ch) != XString<_ChTy>::npos )
        {
            XString<_ChTy> slash;
            switch ( ch )
            {
            case Literal<_ChTy>::belChar: // 响铃(BEL)  07H/7
                slash = Literal<_ChTy>::slash_aStr;
                break;
            case Literal<_ChTy>::bsChar: // 退格符(BS) 08H/8
                slash = Literal<_ChTy>::slash_bStr;
                break;
            case Literal<_ChTy>::htChar: // 水平制表符(HT) 09H/9
                slash = Literal<_ChTy>::slash_tStr;
                break;
            case Literal<_ChTy>::lfChar: // 换行符(LF) 0AH/10
                slash = Literal<_ChTy>::slash_nStr;
                break;
            case Literal<_ChTy>::vtChar: // 垂直制表(VT) 0BH/11
                slash = Literal<_ChTy>::slash_vStr;
                break;
            case Literal<_ChTy>::ffChar: // 换页符(FF) 0CH/12
                slash = Literal<_ChTy>::slash_fStr;
                break;
            case Literal<_ChTy>::crChar: // 回车符(CR) 0DH/13
                slash = Literal<_ChTy>::slash_rStr;
                break;
            default:
                {
                    using MyUT = typename UT<sizeof(_ChTy)>::type;
                    if ( IsSpecial(ch) || ch <= Literal<_ChTy>::spaceChar || (MyUT)ch > (MyUT)0x7f )
                    {
                        slash = Literal<_ChTy>::slash_xStr + CharToHexStr(ch);
                    }
                    else
                    {
                        slash += Literal<_ChTy>::slashChar;
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

WINUX_FUNC_IMPL(AnsiString) AddSlashesA( AnsiString const & str, AnsiString const & charlist )
{
    return Impl_AddSlashes( str, charlist );
}

WINUX_FUNC_IMPL(UnicodeString) AddSlashesW( UnicodeString const & str, UnicodeString const & charlist )
{
    return Impl_AddSlashes( str, charlist );
}

/*
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
                        #if defined(CL_MINGW)
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
//*/

template < typename _ChTy >
inline static XString<_ChTy> Impl_StripSlashes( XString<_ChTy> const & str, XString<_ChTy> const & charlist )
{
    using MyConstIterator = typename XString<_ChTy>::const_iterator;
    using MyUT = typename UT< sizeof(_ChTy) >::type;

    XString<_ChTy> result;
    int octMaxLen = (int)ceil( logl((MyUT)(-1)) / logl(8) );
    int hexMaxLen = (int)ceil( logl((MyUT)(-1)) / logl(16) );

    for ( MyConstIterator it = str.begin(); it != str.end(); )
    {
        _ChTy const current = *it;
        if ( current == Literal<_ChTy>::slashChar )
        {
            it++; // skip '\\'
            if ( it != str.end() ) // \后有字符
            {
                for ( ; it != str.end(); it++ )
                {
                    _ChTy ch0 = *it; // ch0表示\后的一个字符
                    if ( IsOct(ch0) )
                    {
                        XString<_ChTy> ch0s;
                        for ( ; it != str.end(); it++ )
                        {
                            ch0 = *it;
                            if ( IsOct(ch0) && ch0s.length() < octMaxLen )
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
                            _ChTy c1 = NumberStringToChar( ch0s.c_str(), 8 );
                            if ( InCharList( c1, charlist ) )
                            {
                                result += c1;
                            }
                            else
                            {
                                result += Literal<_ChTy>::slashChar;
                                result += ch0s;
                            }
                        }

                        break;
                    }
                    else if ( ch0 == Literal<_ChTy>::xChar ) // is x 16进制
                    {
                        it++; // skip 'x'
                        if ( it != str.end() )
                        {
                            if ( IsHex(*it) )
                            {
                                XString<_ChTy> ch0s;
                                for ( ; it != str.end(); it++ )
                                {
                                    ch0 = *it;
                                    if ( IsHex(ch0) && ch0s.length() < hexMaxLen )
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
                                    _ChTy c2 = NumberStringToChar( ch0s.c_str(), 16 );
                                    if ( InCharList( c2, charlist ) )
                                    {
                                        result += c2;
                                        break;
                                    }
                                    else
                                    {
                                        result += Literal<_ChTy>::slash_xStr;
                                        result += ch0s;
                                        break;
                                    }
                                }
                                break;
                            }
                            else // x后不是16进制字符
                            {
                                result += Literal<_ChTy>::slash_xStr;
                                break;
                            }
                        }
                        else // 后面已经没有字符
                        {
                            result += Literal<_ChTy>::slash_xStr;
                            break;
                        }
                    }
                    else if ( IsSpecial(ch0) )
                    {
                        _ChTy special = SpecialToChar(ch0);
                        // 查看CharList是否含有此字符
                        if ( InCharList( special, charlist ) )
                        {
                            result += special;
                            it++; // skip ch0
                            break;
                        }
                        else
                        {
                            result += Literal<_ChTy>::slashChar;
                            result += ch0;
                            it++; // skip ch0
                            break;
                        }
                    }
                    else if ( InCharList( ch0, charlist ) )
                    {
                        result += ch0;
                        it++; // skip ch0
                        break;
                    }
                    else // 都不是
                    {
                        result += Literal<_ChTy>::slashChar;
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

WINUX_FUNC_IMPL(AnsiString) StripSlashesA( AnsiString const & str, AnsiString const & charlist )
{
    return Impl_StripSlashes( str, charlist );
}

WINUX_FUNC_IMPL(UnicodeString) StripSlashesW( UnicodeString const & str, UnicodeString const & charlist )
{
    return Impl_StripSlashes( str, charlist );
}

/*WINUX_FUNC_IMPL(AnsiString) StripSlashes( AnsiString const & str, AnsiString const & charlist )
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
                            char c1 = NumberStringToChar( ch0s.c_str(), 8 );
                            if ( InCharList( c1, charlist ) )
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
                                    char c2 = NumberStringToChar( ch0s.c_str(), 16 );
                                    if ( InCharList( c2, charlist ) )
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
                        if ( InCharList( special, charlist ) )
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
                    else if ( InCharList( ch0, charlist ) )
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
//*/

template < typename _ChTy >
inline static XString<_ChTy> Impl_AddQuotes( XString<_ChTy> const & str, _ChTy quote )
{
    XString<_ChTy> res;
    using MyConstIterator = typename XString<_ChTy>::const_iterator;
    MyConstIterator it;
    for ( it = str.begin(); it != str.end(); ++it )
    {
        if ( *it == quote )
        {
            res += XString<_ChTy>( 1, quote ) + *it;
        }
        else
        {
            res += *it;
        }
    }
    return res;
}

WINUX_FUNC_IMPL(AnsiString) AddQuotesA( AnsiString const & str, AnsiString::value_type quote )
{
    return Impl_AddQuotes( str, quote );
}

WINUX_FUNC_IMPL(UnicodeString) AddQuotesW( UnicodeString const & str, UnicodeString::value_type quote )
{
    return Impl_AddQuotes( str, quote );
}

template < typename _ChTy >
inline static bool Impl_StrGetLine( XString<_ChTy> * line, XString<_ChTy> const & str, int * i, XString<_ChTy> * nl )
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
        _ChTy ch = str[*i];
        switch ( ch )
        {
        case Literal<_ChTy>::lfChar:
        case Literal<_ChTy>::crChar:
            {
                if ( *i - start > 0 )
                {
                    ASSIGN_PTR(line) = str.substr( start, *i - start ); // 不包括 \r \n
                }

                switch ( ch )
                {
                case Literal<_ChTy>::lfChar:
                    ASSIGN_PTR(nl) += ch;
                    ++ *i; // skip '\n'
                    break;
                case Literal<_ChTy>::crChar:
                    ASSIGN_PTR(nl) += ch;
                    ++ *i; // skip '\r'
                    if ( *i < (int)str.length() && ( ch = str[*i] ) == Literal<_ChTy>::lfChar )
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

WINUX_FUNC_IMPL(bool) StrGetLineA( AnsiString * line, AnsiString const & str, int * i, AnsiString * nl )
{
    return Impl_StrGetLine( line, str, i, nl );
}

WINUX_FUNC_IMPL(bool) StrGetLineW( UnicodeString * line, UnicodeString const & str, int * i, UnicodeString * nl )
{
    return Impl_StrGetLine( line, str, i, nl );
}

template < typename _ChTy >
inline static XString<_ChTy> & Impl_CollateWord( XString<_ChTy> * word, winux::uint flags )
{
    if ( word->empty() ) return *word;
    switch ( flags & 0xF )
    {
    case wordAllUpper:
        Impl_StrMakeUpper(word);
        break;
    case wordAllLower:
        Impl_StrMakeLower(word);
        break;
    case wordFirstCharUpper:
        Impl_StrMakeLower(word);
        if ( IsLowerAlphabet( word->at(0) ) )
        {
            word->at(0) &= ~0x20;
        }
        break;
    default:
        break;
    }
    return *word;
}

template < typename _ChTy >
inline static void Impl_StoreWordToIdentifierString( XString<_ChTy> * resIdentifier, XStringArray<_ChTy> * resWords, XString<_ChTy> & tmpWord, XString<_ChTy> const & sep, winux::uint flags )
{
    Impl_CollateWord( &tmpWord, flags );
    if ( resIdentifier->empty() && ( flags & 0xF0 ) == nameSmallHump )
    {
        if ( !tmpWord.empty() ) tmpWord[0] |= 0x20;
        *resIdentifier += tmpWord + sep;
    }
    else
    {
        *resIdentifier += tmpWord + sep;
    }
}

template < typename _ChTy >
inline static void Impl_StoreWordToArray( XString<_ChTy> * resIdentifier, XStringArray<_ChTy> * resWords, XString<_ChTy> & tmpWord, XString<_ChTy> const & sep, winux::uint flags )
{
    Impl_CollateWord( &tmpWord, flags );
    if ( resWords->empty() && ( flags & 0xF0 ) == nameSmallHump )
    {
        if ( !tmpWord.empty() ) tmpWord[0] |= 0x20;
        resWords->push_back(tmpWord);
    }
    else
    {
        resWords->push_back(tmpWord);
    }
}

enum _StoreWordType
{
    storeIdentifier,
    storeArray
};

template < typename _ChTy >
inline static void Impl_CollateIdentifierToX(
    XString<_ChTy> const & identifier,
    XString<_ChTy> const & sep,
    winux::uint flags,
    _StoreWordType storeType,
    XString<_ChTy> * resIdentifier,
    XStringArray<_ChTy> * resWords
)
{
    XString<_ChTy> tmpWord;

    /* 拆词算法 */
    for ( size_t i = 0; i < identifier.length(); )
    {
        _ChTy ch = identifier[i];

        if ( IsUpperAlphabet(ch) ) // 是大写字母
        {
            if ( tmpWord.empty() )
            {
                tmpWord += ch;
                i++;
            }
            else
            {
                if ( i + 1 < identifier.length() )
                {
                    if ( IsLowerAlphabet(identifier[i + 1]) ) // 后一个字符是小写字符，则说明该字符是一个单词的开头。
                    {
                        switch ( storeType )
                        {
                        case winux::storeIdentifier:
                            Impl_StoreWordToIdentifierString( resIdentifier, resWords, tmpWord, sep, flags );
                            break;
                        case winux::storeArray:
                            Impl_StoreWordToArray( resIdentifier, resWords, tmpWord, sep, flags );
                            break;
                        }
                        tmpWord = ch;
                    }
                    else
                    {
                        if ( IsLowerAlphabet(tmpWord[tmpWord.length() - 1]) )
                        {
                            switch ( storeType )
                            {
                            case winux::storeIdentifier:
                                Impl_StoreWordToIdentifierString( resIdentifier, resWords, tmpWord, sep, flags );
                                break;
                            case winux::storeArray:
                                Impl_StoreWordToArray( resIdentifier, resWords, tmpWord, sep, flags );
                                break;
                            }
                            tmpWord = ch;
                        }
                        else
                        {
                            tmpWord += ch;
                        }
                    }
                }
                else
                {
                    tmpWord += ch;
                }

                i++;
            }
        }
        else if ( IsLowerAlphabet(ch) ) // 是小写字母
        {
            tmpWord += ch;
            i++;
        }
        else if ( IsDelimChar(ch) ) // 是分隔符
        {
            switch ( storeType )
            {
            case winux::storeIdentifier:
                Impl_StoreWordToIdentifierString( resIdentifier, resWords, tmpWord, sep, flags );
                break;
            case winux::storeArray:
                Impl_StoreWordToArray( resIdentifier, resWords, tmpWord, sep, flags );
                break;
            }
            tmpWord.clear();
            i++;
        }
        else // 是其他字符 
        {
            tmpWord += ch;
            i++;
        }
    }

    if ( !tmpWord.empty() )
    {
        switch ( storeType )
        {
        case winux::storeIdentifier:
            Impl_StoreWordToIdentifierString<_ChTy>( resIdentifier, resWords, tmpWord, Literal<_ChTy>::emptyStr, flags );
            break;
        case winux::storeArray:
            Impl_StoreWordToArray<_ChTy>( resIdentifier, resWords, tmpWord, Literal<_ChTy>::emptyStr, flags );
            break;
        }
    }
}

template < typename _ChTy >
inline static XString<_ChTy> Impl_CollateIdentifierToString( XString<_ChTy> const & identifier, XString<_ChTy> const & sep, winux::uint flags )
{
    XString<_ChTy> resIdentifier;
    Impl_CollateIdentifierToX<_ChTy>( identifier, sep, flags, storeIdentifier, &resIdentifier, nullptr );
    return resIdentifier;
}

WINUX_FUNC_IMPL(AnsiString) CollateIdentifierToStringA( AnsiString const & identifier, AnsiString const & sep, winux::uint flags )
{
    return Impl_CollateIdentifierToString( identifier, sep, flags );
}

WINUX_FUNC_IMPL(UnicodeString) CollateIdentifierToStringW( UnicodeString const & identifier, UnicodeString const & sep, winux::uint flags )
{
    return Impl_CollateIdentifierToString( identifier, sep, flags );
}

template < typename _ChTy >
inline static XStringArray<_ChTy> Impl_CollateIdentifierToArray( XString<_ChTy> const & identifier, winux::uint flags )
{
    XStringArray<_ChTy> resWords;
    Impl_CollateIdentifierToX<_ChTy>( identifier, Literal<_ChTy>::emptyStr, flags, storeArray, nullptr, &resWords );
    return resWords;
}

WINUX_FUNC_IMPL(AnsiStringArray) CollateIdentifierToArrayA( AnsiString const & identifier, winux::uint flags )
{
    return Impl_CollateIdentifierToArray( identifier, flags );
}

WINUX_FUNC_IMPL(UnicodeStringArray) CollateIdentifierToArrayW( UnicodeString const & identifier, winux::uint flags )
{
    return Impl_CollateIdentifierToArray( identifier, flags );
}

// -----------------------------------------------------------------------------------------
WINUX_FUNC_IMPL(std::vector<int>) KmpCalcNext( char const * substr, int sublen )
{
    return _Templ_KmpCalcNext< char, int >( substr, sublen );
}

/* KMP匹配 */
WINUX_FUNC_IMPL(int) KmpMatchEx( char const * str, int len, char const * substr, int sublen, int pos, std::vector<int> const & next )
{
    return _Templ_KmpMatchEx< char, int >( str, len, substr, sublen, pos, next );
}

WINUX_FUNC_IMPL(int) KmpMatch( char const * str, int len, char const * substr, int sublen, int pos )
{
    return KmpMatchEx( str, len, substr, sublen, pos, KmpCalcNext( substr, sublen ) );
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
        _nextVals.push_back( _Templ_KmpCalcNext< StringArray::value_type::value_type, short >( it->c_str(), (int)it->length() ) );
    }
    _replaceFunc = NULL;
    _extra = NULL;
}

void MultiMatch::init( StringArray const & matches, ReplaceFuncType replaceFunc, void * extra )
{
    _matchItems = matches;

    for ( StringArray::const_iterator it = _matchItems.begin(); it != _matchItems.end(); it++ )
    {
        _nextVals.push_back( _Templ_KmpCalcNext< StringArray::value_type::value_type, short >( it->c_str(), (int)it->length() ) );
    }
    _replaceFunc = replaceFunc;
    _extra = extra;
}

int MultiMatch::addMatchReplacePair( String const & match, String const & replace )
{
    _matchItems.push_back(match);
    _nextVals.push_back( _Templ_KmpCalcNext< String::value_type, short >( match.c_str(), (int)match.length() ) );
    _replaceItems.push_back(replace);
    return (int)_nextVals.size();
}

int MultiMatch::addMatch( String const & match )
{
    _matchItems.push_back(match);
    _nextVals.push_back( _Templ_KmpCalcNext< String::value_type, short >( match.c_str(), (int)match.length() ) );
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
    int count = (int)_matchItems.size();
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
        int matchItemCount = (int)_matchItems.size();
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
        offset += r.pos + (int)_matchItems[r.item].length();
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

#if !defined(OS_WIN)
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

WINUX_FUNC_IMPL(uint) LocalCharsCount( AnsiString const & local )
{
    SetLocale __setLoc;
    uint cnt = (uint)mbstowcs( NULL, local.c_str(), local.length() );
    if ( cnt != (uint)-1 ) return cnt;
#if defined(_MSC_VER) || defined(WIN32)
    return (uint)_mbslen( (unsigned char *)local.c_str() );
#else
    return (uint)mbslen( local.c_str(), local.length() );
#endif
}

WINUX_FUNC_IMPL(uint) UnicodeMinLength( UnicodeString const & unicode )
{
    SetLocale __setLoc;
    uint len = (uint)wcstombs( NULL, unicode.c_str(), 0 );
    if ( len != (uint)-1 ) return len;
    len = 0;
    UnicodeString::const_iterator it;
    for ( it = unicode.begin(); it != unicode.end(); ++it )
        len += ((unsigned int)*it) > 0xFFU ? 2 : 1;
    return len;
}

WINUX_FUNC_IMPL(AnsiString) UnicodeToLocal( UnicodeString const & unicode )
{
    SetLocale __setLoc;
    AnsiString r;
    r.resize( UnicodeMinLength(unicode) + 1 );
    wcstombs( &r[0], unicode.c_str(), r.size() );
    return r.c_str();
}

WINUX_FUNC_IMPL(UnicodeString) LocalToUnicode( AnsiString const & local )
{
    SetLocale __setLoc;
    UnicodeString r;
    r.resize( LocalCharsCount(local) + 1 );
    mbstowcs( &r[0], local.c_str(), local.length() );
    return r.c_str();
}

WINUX_FUNC_IMPL(String) LocalToString( AnsiString const & local )
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

WINUX_FUNC_IMPL(AnsiString) StringToLocal( String const & str )
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

AnsiString FormatExVA( uint cch, char const * fmt, va_list args )
{
    AnsiString str;
    str.resize( cch + 1 );
    _vsnprintf( &str[0], cch, fmt, args );
    return str.c_str();
}

UnicodeString FormatExVW( uint cch, wchar const * fmt, va_list args )
{
    UnicodeString str;
    str.resize( cch + 1 );
    _vsnwprintf( &str[0], cch, fmt, args );
    return str.c_str();
}

AnsiString FormatExA( uint cch, char const * fmt, ... )
{
    va_list args;
    va_start( args, fmt );
    return FormatExVA( cch, fmt, args );
}

UnicodeString FormatExW( uint cch, wchar const * fmt, ... )
{
    va_list args;
    va_start( args, fmt );
    return FormatExVW( cch, fmt, args );
}

AnsiString FormatA( char const * fmt, ... )
{
    va_list args;
    va_start( args, fmt );

#if defined(_MSC_VER) || defined(WIN32)
    int c = _vscprintf( fmt, args );
    return FormatExVA( c, fmt, args );
#else
    char * buf = NULL;
    vasprintf( &buf, fmt, args );
    AnsiString s = buf;
    free(buf);
    return s;
#endif
}

UnicodeString FormatW( wchar const * fmt, ... )
{
    va_list args;
    va_start( args, fmt );

#if defined(_MSC_VER) || defined(WIN32)
    int c = _vscwprintf( fmt, args );
    return FormatExVW( c, fmt, args );
#else
    int c = 4096;
    return FormatExVW( c, fmt, args );
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
            AnsiString str = UnicodeToLocal(pwstr);
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
#if defined(__GNUC__) || defined(HAVE_ICONV_H)
    //转换句柄
    iconv_t _cd;
#else
    uint _fromCP;
    uint _toCP;
#endif

};

// class Conv -----------------------------------------------------------------------------
#if !defined(__GNUC__) && !defined(HAVE_ICONV_H)
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
        //LOG( Format("GetLastError() %X", GetLastError() ) );
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

#if defined(__GNUC__) || defined(HAVE_ICONV_H)
    _self->_cd = iconv_open( toCode, fromCode );

    #if _LIBICONV_VERSION > 0x0108
    int optval;
    // 字译功能
    optval = 1;
    iconvctl( _self->_cd, ICONV_SET_TRANSLITERATE, &optval );
    // 丢弃无效序列
    optval = 1;
    iconvctl( _self->_cd, ICONV_SET_DISCARD_ILSEQ, &optval );
    #endif

#else
    _self->_fromCP = __LangCP[fromCode];
    _self->_toCP = __LangCP[toCode];
#endif
}

Conv::~Conv()
{
#if defined(__GNUC__) || defined(HAVE_ICONV_H)
    iconv_close(_self->_cd);
#endif

    _self.destroy(); //
}

int Conv::convert( char const * srcBuf, size_t srcSize, char * * destBuf )
{
#if defined(__GNUC__) || defined(HAVE_ICONV_H)
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
        r = (int)iconv( _self->_cd, &srcP, &srcN, &buf, &outBytesLeft );
        outBytes = (int)( destSize - outBytesLeft ); // 求得输出的字节数
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
