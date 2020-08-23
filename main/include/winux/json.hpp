#ifndef __JSON_HPP__
#define __JSON_HPP__

namespace winux
{

/** \brief Json解析成Mixed */
WINUX_FUNC_DECL(bool) JsonParse( String const & json, Mixed * val );

/** \brief Mixed输出成Json */
WINUX_FUNC_DECL(AnsiString) MixedToJsonA( Mixed const & v, bool autoKeyQuotes );
WINUX_FUNC_DECL(UnicodeString) MixedToJsonW( Mixed const & v, bool autoKeyQuotes );

#if defined(_UNICODE) || defined(UNICODE)
    #define MixedToJson MixedToJsonW
#else
    #define MixedToJson MixedToJsonA
#endif

}

#endif
