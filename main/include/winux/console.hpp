#ifndef __CONSOLE_HPP__
#define __CONSOLE_HPP__

#include <iostream>

namespace winux
{

/** \brief 颜色属性标记 */
enum ConsoleColorAttrFlags : winux::ushort
{
#if defined(OS_WIN)
    fgBlack = 0,
    bgBlack = 0,

    fgNavy = 0x0001,
    fgAtrovirens = 0x0002,
        fgTeal = fgNavy | fgAtrovirens,
    fgMaroon = 0x0004,
        fgPurple = fgNavy | fgMaroon,
        fgOlive = fgAtrovirens | fgMaroon,
        fgSilver = fgNavy | fgAtrovirens | fgMaroon,

    fgIntensity = 0x0008,
    fgGray = fgIntensity,

    fgBlue = fgIntensity | fgNavy,
    fgGreen = fgIntensity | fgAtrovirens,
        fgAqua = fgIntensity | fgNavy | fgAtrovirens,
    fgRed = fgIntensity | fgMaroon,
        fgFuchsia = fgIntensity | fgNavy | fgMaroon,
        fgYellow = fgIntensity | fgAtrovirens | fgMaroon,
        fgWhite = fgIntensity | fgNavy | fgAtrovirens | fgMaroon,

////////////////////////////////////////////////////////////////
    bgNavy = 0x0010,
    bgAtrovirens = 0x0020,
        bgTeal = bgNavy | bgAtrovirens,
    bgMaroon = 0x0040,
        bgPurple = bgNavy | bgMaroon,
        bgOlive = bgAtrovirens | bgMaroon,
        bgSilver = bgNavy | bgAtrovirens | bgMaroon,

    bgIntensity = 0x0080,
    bgGray = bgIntensity,

    bgBlue = bgIntensity | bgNavy,
    bgGreen = bgIntensity | bgAtrovirens,
        bgAqua = bgIntensity | bgNavy | bgAtrovirens,
    bgRed = bgIntensity | bgMaroon,
        bgFuchsia = bgIntensity | bgNavy | bgMaroon,
        bgYellow = bgIntensity | bgAtrovirens | bgMaroon,
        bgWhite = bgIntensity | bgNavy | bgAtrovirens | bgMaroon,
#else
    fgBlack = 0,
    fgNavy = 1,
    fgAtrovirens = 2,
    fgTeal = 3,
    fgMaroon = 4,
    fgPurple = 5,
    fgOlive = 6,
    fgSilver = 7,
    fgGray = 8,
    fgIntensity = fgGray,
    fgBlue = 9,
    fgGreen = 10,
    fgAqua = 11,
    fgRed = 12,
    fgFuchsia = 13,
    fgYellow = 14,
    fgWhite = 15,

    bgNavy = 0x0100,
    bgAtrovirens = 0x0200,
    bgTeal = 0x0300,
    bgMaroon = 0x0400,
    bgPurple = 0x0500,
    bgOlive = 0x0600,
    bgSilver = 0x0700,
    bgBlack = 0x0800,
    bgWhite = 0x0000,
    bgGray = bgSilver,
    bgBlue = bgNavy,
    bgGreen = bgAtrovirens,
    bgAqua = bgTeal,
    bgRed = bgMaroon,
    bgFuchsia = bgPurple,
    bgYellow = bgOlive,
#endif

};

#if defined(OS_WIN)
#else
extern WINUX_DLL char const * __TerminalFgColorAttrs[];
extern WINUX_DLL char const * __TerminalBgColorAttrs[];

#endif

class WINUX_DLL ConsoleAttr
{
private:
#if defined(OS_WIN)
    WORD _wPrevAttributes;
    WORD _wAttributes;
    HANDLE _hStdHandle;
#else
    winux::String _strAttr;
#endif
    bool _isSetBgColor;
public:
    ConsoleAttr( winux::ushort attr, bool isSetBgColor = false );

    void modify() const;

    void resume() const;
};

template < typename _VarType >
class ConsoleAttrT : public ConsoleAttr
{
private:
    _VarType & _v;
public:
    ConsoleAttrT( winux::ushort attr, _VarType const & v, bool isSetBgColor = false ) : ConsoleAttr( attr, isSetBgColor ), _v( const_cast<_VarType&>(v) )
    {
    }

    _VarType & val() const { return _v; }

};

template < typename _VarType >
inline std::ostream & operator << ( std::ostream & o, ConsoleAttrT<_VarType> const & tr )
{
    tr.modify();
    o << tr.val();
    tr.resume();
    return o;
}

template < typename _VarType >
inline std::istream & operator >> ( std::istream & in, ConsoleAttrT<_VarType> const & tr )
{
    tr.modify();
    in >> tr.val();
    tr.resume();
    return in;
}

template < typename _VarType >
inline ConsoleAttrT<_VarType> ConsoleColor( winux::ushort attr, _VarType const & v, bool isSetBgColor = false )
{
    return ConsoleAttrT<_VarType>( attr, v, isSetBgColor );
}

class WINUX_DLL ConsoleOuputMutexScopeGuard
{
public:
    ConsoleOuputMutexScopeGuard();
    ~ConsoleOuputMutexScopeGuard();
private:
    DISABLE_OBJECT_COPY(ConsoleOuputMutexScopeGuard)
};

inline static void OutputV()
{
}

template < typename _Ty, typename... _ArgType >
inline static void OutputV( _Ty&& a, _ArgType&& ... arg )
{
    std::cout << a;
    OutputV( std::forward<_ArgType>(arg)... );
}

template < typename... _ArgType >
inline static void ColorOutputLine( winux::ConsoleAttr const & ca, _ArgType&& ... arg )
{
    ConsoleOuputMutexScopeGuard guard;
    ca.modify();
    OutputV( std::forward<_ArgType>(arg)... );
    ca.resume();
    std::cout << std::endl;
}

template < typename... _ArgType >
inline static void ColorOutput( winux::ConsoleAttr const & ca, _ArgType&& ... arg )
{
    ConsoleOuputMutexScopeGuard guard;
    ca.modify();
    OutputV( std::forward<_ArgType>(arg)... );
    ca.resume();
}

} // namespace winux

#endif // __CONSOLE_HPP__
