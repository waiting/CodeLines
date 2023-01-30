#ifndef __JSON_HPP__
#define __JSON_HPP__

namespace winux
{

/** \brief 设置字节序。此字节序是解析\uHHHH时，存储编码数字的字节序。默认小端序。 */
WINUX_FUNC_DECL(bool) JsonSetByteOrderForUtf16( bool isLittleEndian );

/** \brief 设置转换到的字符集编码。此表示解析\uHHHH时，转换成什么编码。默认为空串，即本地编码。
 *
 *  假设JSON的编码是X，当X不是本地编码，并且内部可能有非ANSI字符以及\uHHHH的字符时，应该调用此函数指定编码为X。不然\uHHHH会转换成本地编码和JSON的编码X冲突。
 *  \param charset 必须是iconv库能识别的编码字符串，例如UTF-8、UTF-16、UTF-16LE、UTF-16BE、GBK等等。 */
WINUX_FUNC_DECL(String) JsonSetConvertToCharsetForUtf16( String const & charset );

/** \brief Json解析成Mixed */
WINUX_FUNC_DECL(bool) JsonParse( String const & json, Mixed * val );

/** \brief Json解析成Mixed */
WINUX_FUNC_DECL(Mixed) Json( String const & json );

/** \brief Mixed输出成Json */
WINUX_FUNC_DECL(AnsiString) MixedToJsonA( Mixed const & v, bool autoKeyQuotes );
/** \brief Mixed输出成Json */
WINUX_FUNC_DECL(UnicodeString) MixedToJsonW( Mixed const & v, bool autoKeyQuotes );

/** \brief Mixed输出成Json，支持结构化。
 *
 *  \param spacer 缩进留白，默认空串表示不缩进留白
 *  \param newline 换行符，默认空串表示不换行 */
WINUX_FUNC_DECL(AnsiString) MixedToJsonExA( Mixed const & v, bool autoKeyQuotes, AnsiString const & spacer = "", AnsiString const & newline = "" );
/** \brief Mixed输出成Json，支持结构化。
 *
 *  \param spacer 缩进留白，默认空串表示不缩进留白
 *  \param newline 换行符，默认空串表示不换行 */
WINUX_FUNC_DECL(UnicodeString) MixedToJsonExW( Mixed const & v, bool autoKeyQuotes, UnicodeString const & spacer = L"", UnicodeString const & newline = L"" );

#if defined(_UNICODE) || defined(UNICODE)
    #define MixedToJson MixedToJsonW
#else
    #define MixedToJson MixedToJsonA
#endif

}

#endif
