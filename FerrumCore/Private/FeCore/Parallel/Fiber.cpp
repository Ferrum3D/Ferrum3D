#include <FeCore/Logging/Trace.h>
#include <FeCore/Memory/Memory.h>
#include <FeCore/Parallel/Fiber.h>

namespace FE::Threading
{
    inline static constexpr size_t NormalStackSize = 512 * 1024;
    inline static constexpr size_t ExtendedStackSize = 512 * 1024;
    inline static constexpr uint32_t NormalFiberCount = 128;
    inline static constexpr uint32_t ExtendedFiberCount = 32;
    inline static constexpr uint32_t TotalFiberCount = NormalFiberCount + ExtendedFiberCount;
    inline static constexpr size_t TotalStackSize = NormalFiberCount * NormalStackSize + ExtendedFiberCount * ExtendedStackSize;


    FiberPool::FiberPool(Context::Callback fiberCallback)
    {
        const Memory::PlatformSpec memorySpec = Memory::GetPlatformSpec();
        const size_t totalGuardPagesSize = (TotalFiberCount + 1) * memorySpec.m_pageSize;

        m_stackMemorySize = AlignUp(totalGuardPagesSize + TotalStackSize, memorySpec.m_granularity);
        m_stackMemory = Memory::AllocateVirtual(m_stackMemorySize);

        m_fibers = Memory::DefaultAllocateArray<FiberInfo>(TotalFiberCount);
        eastl::uninitialized_default_fill(m_fibers, m_fibers + TotalFiberCount);

        std::byte* ptr = static_cast<std::byte*>(m_stackMemory);
        for (uint32_t i = 0; i < NormalFiberCount; ++i)
        {
            Memory::ProtectVirtual(ptr, memorySpec.m_pageSize, Memory::ProtectFlags::kNone);
            ptr += memorySpec.m_pageSize + NormalStackSize; // top of the stack
            m_fibers[i].m_isFree = true;
            m_fibers[i].m_context = Context::Create(ptr, NormalStackSize, fiberCallback);
            Fmt::FormatTo(m_fibers[i].m_name, "Fiber {}", i);
        }
        for (uint32_t i = 0; i < ExtendedFiberCount; ++i)
        {
            Memory::ProtectVirtual(ptr, memorySpec.m_pageSize, Memory::ProtectFlags::kNone);
            ptr += memorySpec.m_pageSize + ExtendedStackSize;
            m_fibers[i + NormalFiberCount].m_isFree = true;
            m_fibers[i + NormalFiberCount].m_context = Context::Create(ptr, ExtendedStackSize, fiberCallback);
            Fmt::FormatTo(m_fibers[i + NormalFiberCount].m_name, "Fiber Big {}", i);
        }

        Memory::ProtectVirtual(ptr, memorySpec.m_pageSize, Memory::ProtectFlags::kNone);
        FE_Assert(ptr + memorySpec.m_pageSize == static_cast<std::byte*>(m_stackMemory) + totalGuardPagesSize + TotalStackSize);
    }


    FiberPool::~FiberPool()
    {
        Memory::DefaultFree(m_fibers);
    }


    FiberHandle FiberPool::Rent(bool extended)
    {
        const uint32_t fiberCount = extended ? ExtendedFiberCount : NormalFiberCount;
        const uint32_t baseFiberIndex = extended ? NormalFiberCount : 0;
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


    void FiberPool::Return(FiberHandle fiberHandle)
    {
        m_fibers[fiberHandle.m_value].m_isFree.store(true, std::memory_order_release);
    }


    void FiberPool::Update(FiberHandle fiberHandle, Context::Handle context)
    {
        if (fiberHandle)
            m_fibers[fiberHandle.m_value].m_context = context;
    }


    Context::TransferParams FiberPool::Switch(FiberHandle to, uintptr_t userData)
    {
        TracyFiberEnter(m_fibers[to.m_value].m_name.Data());
        return Context::Switch(m_fibers[to.m_value].m_context, userData);
    }
} // namespace FE::Threading
