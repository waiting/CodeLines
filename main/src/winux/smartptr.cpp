#ifndef __GNUC__
#pragma warning( disable: 4786 )
#endif

#include "utilities.hpp"
#include "system.hpp"
#include "smartptr.hpp"

namespace winux
{

static MutexLockObj __refCountAtomic;

WINUX_FUNC_IMPL(long) LongAtomicIncrement( long volatile * p )
{
    ScopeLock sl(__refCountAtomic);
    return ++*p;
}

WINUX_FUNC_IMPL(long) LongAtomicDecrement( long volatile * p )
{
    ScopeLock sl(__refCountAtomic);
    return --*p;
}


} // namespace winux
