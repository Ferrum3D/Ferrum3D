#pragma once
#include <FeCore/Base/Base.h>
#include <FeCore/Memory/LinearAllocator.h>
#include <FeCore/Memory/PoolAllocator.h>
#include <FeCore/Strings/Format.h>
#include <FeCore/Threading/Context.h>

namespace FE::Threading
{
    struct FiberHandle final : TypedHandle<FiberHandle, uint32_t>
    {
    };


    struct alignas(Memory::kCacheLineSize) FiberRuntimeInfo final
    {
        static constexpr uint32_t kFiberMagic = 0xfefb14f0;

        uint32_t m_magic = kFiberMagic;
        FiberHandle m_handle;
        const char* m_name = nullptr;
        Memory::LinearAllocator m_tempAllocator;

        FiberRuntimeInfo(const uint32_t pageSize, std::pmr::memory_resource* pageAllocator)
            : m_tempAllocator(pageSize, pageAllocator)
        {
        }

        FE_FORCE_NOINLINE static FiberRuntimeInfo& Get();
    };


    struct FiberPool final
    {
        explicit FiberPool(Context::Callback fiberCallback);
        ~FiberPool();

        FiberPool(const FiberPool&) = delete;
        FiberPool& operator=(const FiberPool&) = delete;
        FiberPool(FiberPool&&) = delete;
        FiberPool& operator=(FiberPool&&) = delete;

        FiberHandle Rent(bool extended);
        void Return(FiberHandle fiberHandle);

        void Update(FiberHandle fiberHandle, Context::Handle context);
        Context::TransferParams Switch(FiberHandle to, uintptr_t userData, const char* message);

    private:
        struct alignas(Memory::kCacheLineSize) FiberInfo final
        {
            std::atomic<bool> m_isFree = false;
            festd::basic_fixed_string<116> m_name;
            Context::Handle m_context;
            FiberRuntimeInfo* m_runtimeInfo = nullptr;
        };

        void* m_stackMemory = nullptr;
        size_t m_stackMemorySize = 0;
        FiberInfo* m_fibers = nullptr;
        std::atomic<uint32_t> m_indexSeed;

        Memory::LockedMemoryResource<Memory::PoolAllocator, SpinLock> m_tempPagePool;
    };
} // namespace FE::Threading
