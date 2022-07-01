
#include "utilities.hpp"
#include "system.hpp"
#include "smartptr.hpp"

namespace winux
{

static MutexLockObj __refCountAtomic;

WINUX_FUNC_IMPL(long) LongAtomicIncrement( long volatile * p )
{
    ScopeGuard guard(__refCountAtomic);
    return ++*p;
}

WINUX_FUNC_IMPL(long) LongAtomicDecrement( long volatile * p )
{
    ScopeGuard guard(__refCountAtomic);
    return --*p;
}

WINUX_FUNC_IMPL(long) LongAtomicCompareExchange( long volatile * p, long exchange, long comparand )
{
    ScopeGuard guard(__refCountAtomic);
    long t = *p;
    if ( *p == comparand ) *p = exchange;
    return t;
}

} // namespace winux
