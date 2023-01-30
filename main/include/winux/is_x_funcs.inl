#pragma once

// IsXXX()内联函数（文件级）

// 是否为8进制数字符
inline static bool IsOct( unsigned ch )
{
    return ch >= '0' && ch <= '7';
}

// 是否为16进制数字符
inline static bool IsHex( unsigned ch )
{
    return ( ch >= '0' && ch <= '9' ) || ( ( ch | 0x20 ) >= 'a' && ( ch | 0x20 ) <= 'f' );
}

// 是否为空白字符
inline static bool IsSpace( unsigned ch )
{
    return ch > '\0' && ch <= ' ';
}

// 是否为数字
inline static bool IsDigit( unsigned ch )
{
    return ch >= '0' && ch <= '9';
}

// 是否为标识符合法字，包括美元符，不包括数字
inline static bool IsWord( unsigned ch )
{
    return ( ( ch | 0x20 ) >= 'a' && ( ch | 0x20 ) <= 'z' ) || ch == '_' || ch == '$' || ch > 127U;
}

// 是否为标识符合法字，不包括美元符，不包括数字
inline static bool IsWordNoDollar( unsigned ch )
{
    return ( ( ch | 0x20 ) >= 'a' && ( ch | 0x20 ) <= 'z' ) || ch == '_' || ch > 127U;
}

// 是否为大写字母
inline static bool IsUpperAlphabet( unsigned ch )
{
    return ch >= 'A' && ch <= 'Z';
}

// 是否为小写字母
inline static bool IsLowerAlphabet( unsigned ch )
{
    return ch >= 'a' && ch <= 'z';
}

// 是否为字母
inline static bool IsAlphabet( unsigned ch )
{
    return ( ( ch | 0x20 ) >= 'a' && ( ch | 0x20 ) <= 'z' );
}

// 是否为分隔符
inline static bool IsDelimChar( unsigned ch )
{
    return !( IsDigit(ch) || IsAlphabet(ch) || ch > 127U );
}
