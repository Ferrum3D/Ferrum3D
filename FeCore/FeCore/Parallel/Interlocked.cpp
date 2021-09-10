#include <FeCore/Parallel/Interlocked.h>
#include <FeCore/Base/PlatformInclude.h>

namespace FE
{
    Int32 Interlocked::Add(AtomicInt32& dst, Int32 val)
    {
        return InterlockedAdd(&dst, val);
    }

    Int64 Interlocked::Add(AtomicInt64& dst, Int64 val)
    {
        return InterlockedAdd64(&dst, val);
    }

    Int16 Interlocked::And(AtomicInt16& dst, Int16 val)
    {
        return InterlockedAnd16(&dst, val);
    }

    Int32 Interlocked::And(AtomicInt32& dst, Int32 val)
    {
        return InterlockedAnd(&dst, val);
    }

    Int64 Interlocked::And(AtomicInt64& dst, Int64 val)
    {
        return InterlockedAnd64(&dst, val);
    }

    Int16 Interlocked::Or(AtomicInt16& dst, Int16 val)
    {
        return InterlockedOr16(&dst, val);
    }

    Int32 Interlocked::Or(AtomicInt32& dst, Int32 val)
    {
        return InterlockedOr(&dst, val);
    }

    Int64 Interlocked::Or(AtomicInt64& dst, Int64 val)
    {
        return InterlockedOr64(&dst, val);
    }

    Int16 Interlocked::Exchange(AtomicInt16& dst, Int16 val)
    {
        return InterlockedExchange16(&dst, val);
    }

    Int32 Interlocked::Exchange(AtomicInt32& dst, Int32 val)
    {
        return InterlockedExchange(&dst, val);
    }

    Int64 Interlocked::Exchange(AtomicInt64& dst, Int64 val)
    {
        return InterlockedExchange64(&dst, val);
    }

    Int16 Interlocked::Increment(AtomicInt16& val)
    {
        return InterlockedIncrement16(&val);
    }

    Int32 Interlocked::Increment(AtomicInt32& val)
    {
        return InterlockedIncrement(&val);
    }

    Int64 Interlocked::Increment(AtomicInt64& val)
    {
        return InterlockedIncrement64(&val);
    }

    Int16 Interlocked::Decrement(AtomicInt16& val)
    {
        return InterlockedDecrement16(&val);
    }

    Int32 Interlocked::Decrement(AtomicInt32& val)
    {
        return InterlockedDecrement(&val);
    }

    Int64 Interlocked::Decrement(AtomicInt64& val)
    {
        return InterlockedDecrement64(&val);
    }

    Int16 Interlocked::CompareExchange(AtomicInt16& dst, Int16 exchg, Int16 cmp)
    {
        return InterlockedCompareExchange16(&dst, exchg, cmp);
    }

    Int32 Interlocked::CompareExchange(AtomicInt32& dst, Int32 exchg, Int32 cmp)
    {
        return InterlockedCompareExchange(&dst, exchg, cmp);
    }

    Int64 Interlocked::CompareExchange(AtomicInt64& dst, Int64 exchg, Int64 cmp)
    {
        return InterlockedCompareExchange64(&dst, exchg, cmp);
    }
} // namespace FE
