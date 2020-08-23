#ifndef __CONSOLE_HPP__
#define __CONSOLE_HPP__

#include <iostream>

namespace winux
{

/** \brief 颜色属性标记 */
enum ConsoleColorAttrFlags
{
#if defined(_MSC_VER) || defined(WIN32)
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

#if defined(_MSC_VER) || defined(WIN32)
#else
extern WINUX_DLL char const * __TerminalFgColorAttrs[];
extern WINUX_DLL char const * __TerminalBgColorAttrs[];

#endif

template < typename _VarType >
class ConsoleAttrT
{
private:
#if defined(_MSC_VER) || defined(WIN32)
    WORD _wPrevAttributes;
    WORD _wAttributes;
    HANDLE _hStdHandle;
#else
    winux::String _strAttr;
#endif
    bool _isSetBgColor;
    _VarType & _v;
public:
    ConsoleAttrT( winux::ushort attr, _VarType const & v, bool isSetBgColor = false ) :
        _isSetBgColor(isSetBgColor),
        _v( const_cast<_VarType &>(v) )
    {
#if defined(_MSC_VER) || defined(WIN32)
        _wAttributes = attr;

        CONSOLE_SCREEN_BUFFER_INFO csbi = { 0 };
        _hStdHandle = GetStdHandle(STD_OUTPUT_HANDLE);
        GetConsoleScreenBufferInfo( _hStdHandle, &csbi );
        _wPrevAttributes = csbi.wAttributes;
#else
        union
        {
            winux::byte bAttr[2];
            winux::ushort usAttr;
        } tmp;
        tmp.usAttr = attr;

        this->_strAttr = __TerminalFgColorAttrs[ tmp.bAttr[0] ];
        if ( tmp.bAttr[1] )
        {
            this->_strAttr += ";";
            this->_strAttr += __TerminalBgColorAttrs[ tmp.bAttr[1] ];
        }
#endif
    }

    void modify() const
    {
#if defined(_MSC_VER) || defined(WIN32)
        SetConsoleTextAttribute( _hStdHandle, _wAttributes | ( _wPrevAttributes & ( _isSetBgColor ? 0xFF00 : 0xFFF0 ) ) );
#else
        std::cout << "\033[" << this->_strAttr << "m";
#endif
    }

    void resume() const
    {
#if defined(_MSC_VER) || defined(WIN32)
        SetConsoleTextAttribute( _hStdHandle, _wPrevAttributes );
#else
        std::cout << "\033[0m";
#endif
    }

    _VarType & val() const { return _v; }

    template < typename _ValTy >
    friend std::ostream & operator << ( std::ostream & o, ConsoleAttrT<_ValTy> & tr );
    template < typename _ValTy >
    friend std::istream & operator >> ( std::istream & in, ConsoleAttrT<_ValTy> & tr );

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

} // namespace winux

#endif // __CONSOLE_HPP__
