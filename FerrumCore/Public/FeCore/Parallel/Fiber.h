﻿#pragma once
#include <FeCore/Base/Base.h>
#include <FeCore/Parallel/Context.h>
#include <FeCore/Strings/Format.h>

namespace FE::Threading
{
    struct FiberHandle final : TypedHandle<FiberHandle, uint32_t>
    {
    };


    class FiberPool final
    {
        struct alignas(Memory::kCacheLineSize) FiberInfo final
        {
            std::atomic<bool> m_isFree;
            FixedString<116> m_name;
            Context::Handle m_context;
        };

        void* m_stackMemory = nullptr;
        size_t m_stackMemorySize = 0;
        FiberInfo* m_fibers = nullptr;
        std::atomic<uint32_t> m_indexSeed;

    public:
        FiberPool(Context::Callback fiberCallback);
        ~FiberPool();

        FiberHandle Rent(bool extended);
        void Return(FiberHandle fiberHandle);

        void Update(FiberHandle fiberHandle, Context::Handle context);
        Context::TransferParams Switch(FiberHandle to, uintptr_t userData);
    };
} // namespace FE::Threading
