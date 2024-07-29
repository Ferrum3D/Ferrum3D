#pragma once
#include <FeCore/Base/Base.h>
#include <FeCore/Parallel/Context.h>
#include <FeCore/Strings/Format.h>

namespace FE
{
    struct FiberHandle final : TypedHandle<FiberHandle, uint32_t>
    {
    };


    class FiberPool final
    {
        struct alignas(Memory::CacheLineSize) FiberInfo final
        {
            String Name;
            Context::Handle Context;
            std::atomic<bool> IsFree;
        };

        void* m_pStackMemory = nullptr;
        size_t m_StackMemorySize = 0;
        FiberInfo* m_Fibers = nullptr;
        std::atomic<uint32_t> m_IndexSeed;

    public:
        FiberPool(Context::Callback fiberCallback);
        ~FiberPool();

        FiberHandle Rent(bool extended);
        void Return(FiberHandle fiberHandle);

        void Update(FiberHandle fiberHandle, Context::Handle context);
        Context::TransferParams Switch(FiberHandle to, uintptr_t userData);
    };
} // namespace FE
