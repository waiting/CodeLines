#if _MSC_VER > 1 && _MSC_VER < 1201
#pragma warning( disable: 4786 )
#endif

#include "utilities.hpp"
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


} // namespace winux
