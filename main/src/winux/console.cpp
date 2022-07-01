
#include "utilities.hpp"
#include "smartptr.hpp"
#include "system.hpp"
#include "threads.hpp"
#include "console.hpp"

namespace winux
{

char const * __TerminalFgColorAttrs[] = {
    "30", "34", "32", "36", "31", "35", "33", "37",
    "1;30", "1;34", "1;32", "1;36", "1;31", "1;35", "1;33", "1;37",
};

char const * __TerminalBgColorAttrs[] = {
    "", "44", "42", "46", "41", "45", "43", "47", "40", "48",
};

// class ConsoleAttr ----------------------------------------------------------------------
ConsoleAttr::ConsoleAttr( winux::ushort attr, bool isSetBgColor ) : _isSetBgColor(isSetBgColor)
{
#if defined(OS_WIN)
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

    this->_strAttr = __TerminalFgColorAttrs[tmp.bAttr[0]];
    if ( tmp.bAttr[1] )
    {
        this->_strAttr += ";";
        this->_strAttr += __TerminalBgColorAttrs[tmp.bAttr[1]];
    }
#endif
}

void ConsoleAttr::modify() const
{
#if defined(OS_WIN)
    SetConsoleTextAttribute( _hStdHandle, _wAttributes | ( _wPrevAttributes & ( _isSetBgColor ? 0xFF00 : 0xFFF0 ) ) );
#else
    std::cout << "\033[" << this->_strAttr << "m";
#endif
}

void ConsoleAttr::resume() const
{
#if defined(OS_WIN)
    SetConsoleTextAttribute( _hStdHandle, _wPrevAttributes );
#else
    std::cout << "\033[0m";
#endif
}

winux::MutexLockObj __mtxConsoleOutput;
ConsoleOuputMutexScopeGuard::ConsoleOuputMutexScopeGuard()
{
    __mtxConsoleOutput.lock();
}

ConsoleOuputMutexScopeGuard::~ConsoleOuputMutexScopeGuard()
{
    __mtxConsoleOutput.unlock();
}

} // namespace winux
