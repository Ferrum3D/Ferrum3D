#include <FeCore/Logging/Trace.h>
#include <FeCore/Memory/Memory.h>
#include <FeCore/Threading/Fiber.h>

namespace FE::Threading
{
    namespace
    {
        inline constexpr size_t kNormalStackSize = 512 * 1024;
        inline constexpr size_t kExtendedStackSize = 512 * 1024;
        inline constexpr uint32_t kNormalFiberCount = 128;
        inline constexpr uint32_t kExtendedFiberCount = 32;
        inline constexpr uint32_t kTotalFiberCount = kNormalFiberCount + kExtendedFiberCount;
        inline constexpr size_t kTotalStackSize = kNormalFiberCount * kNormalStackSize + kExtendedFiberCount * kExtendedStackSize;
    } // namespace


    FiberPool::FiberPool(const Context::Callback fiberCallback)
    {
        FE_PROFILER_ZONE();

        const Memory::PlatformSpec memorySpec = Memory::GetPlatformSpec();
        const size_t totalGuardPagesSize = (kTotalFiberCount + 1) * memorySpec.m_pageSize;

        m_stackMemorySize = AlignUp(totalGuardPagesSize + kTotalStackSize, memorySpec.m_granularity);
        m_stackMemory = Memory::AllocateVirtual(m_stackMemorySize);

        m_fibers = Memory::DefaultAllocateArray<FiberInfo>(kTotalFiberCount);
        eastl::uninitialized_default_fill(m_fibers, m_fibers + kTotalFiberCount);

        std::byte* ptr = static_cast<std::byte*>(m_stackMemory);
        for (uint32_t i = 0; i < kNormalFiberCount; ++i)
        {
            Memory::ProtectVirtual(ptr, memorySpec.m_pageSize, Memory::ProtectFlags::kNone);
            ptr += memorySpec.m_pageSize + kNormalStackSize; // top of the stack
            m_fibers[i].m_isFree = true;
            m_fibers[i].m_context = Context::Create(ptr, kNormalStackSize, fiberCallback);
            Fmt::FormatTo(m_fibers[i].m_name, "Fiber {}", i);
        }
        for (uint32_t i = 0; i < kExtendedFiberCount; ++i)
        {
            Memory::ProtectVirtual(ptr, memorySpec.m_pageSize, Memory::ProtectFlags::kNone);
            ptr += memorySpec.m_pageSize + kExtendedStackSize;
            m_fibers[i + kNormalFiberCount].m_isFree = true;
            m_fibers[i + kNormalFiberCount].m_context = Context::Create(ptr, kExtendedStackSize, fiberCallback);
            Fmt::FormatTo(m_fibers[i + kNormalFiberCount].m_name, "Fiber Big {}", i);
        }

        Memory::ProtectVirtual(ptr, memorySpec.m_pageSize, Memory::ProtectFlags::kNone);
        FE_Assert(ptr + memorySpec.m_pageSize == static_cast<std::byte*>(m_stackMemory) + totalGuardPagesSize + kTotalStackSize);
    }


    FiberPool::~FiberPool()
    {
        Memory::DefaultFree(m_fibers);
    }


    FiberHandle FiberPool::Rent(const bool extended)
    {
        const uint32_t fiberCount = extended ? kExtendedFiberCount : kNormalFiberCount;
        const uint32_t baseFiberIndex = extended ? kNormalFiberCount : 0;
        const uint32_t indexSeed = m_indexSeed.fetch_add(1, std::memory_order_release);
        const uint64_t indexOffset = DefaultHash(&indexSeed, sizeof(indexSeed));

        while (true)
        {
            for (uint32_t fiberIndex = 0; fiberIndex < fiberCount; ++fiberIndex)
            {
                const uint32_t realIndex = static_cast<uint32_t>((fiberIndex + indexOffset) % fiberCount);
                FiberInfo& info = m_fibers[realIndex + baseFiberIndex];
                if (!info.m_isFree.load(std::memory_order_relaxed))
                    continue;

                bool expected = true;
                if (info.m_isFree.compare_exchange_weak(expected, false, std::memory_order_release))
                    return FiberHandle{ realIndex + baseFiberIndex };
            }

            for (uint32_t spin = 0; spin < 32; ++spin)
                _mm_pause();
        }
    }


    void FiberPool::Return(const FiberHandle fiberHandle)
    {
        m_fibers[fiberHandle.m_value].m_isFree.store(true, std::memory_order_release);
    }


    void FiberPool::Update(const FiberHandle fiberHandle, const Context::Handle context)
    {
        if (fiberHandle)
            m_fibers[fiberHandle.m_value].m_context = context;
    }


    Context::TransferParams FiberPool::Switch(const FiberHandle to, const uintptr_t userData, const char* message)
    {
        TracyMessageL(message);
        TracyFiberEnter(m_fibers[to.m_value].m_name.data());
        TracyMessageL(message);
        return Context::Switch(m_fibers[to.m_value].m_context, userData);
    }
} // namespace FE::Threading
