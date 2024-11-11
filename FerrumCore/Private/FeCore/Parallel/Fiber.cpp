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
        const size_t totalGuardPagesSize = (TotalFiberCount + 1) * memorySpec.PageSize;

        m_StackMemorySize = AlignUp(totalGuardPagesSize + TotalStackSize, memorySpec.Granularity);
        m_pStackMemory = Memory::AllocateVirtual(m_StackMemorySize);

        m_Fibers = Memory::DefaultAllocateArray<FiberInfo>(TotalFiberCount);
        eastl::uninitialized_default_fill(m_Fibers, m_Fibers + TotalFiberCount);

        std::byte* ptr = static_cast<std::byte*>(m_pStackMemory);
        for (uint32_t i = 0; i < NormalFiberCount; ++i)
        {
            Memory::ProtectVirtual(ptr, memorySpec.PageSize, Memory::ProtectFlags::None);
            ptr += memorySpec.PageSize + NormalStackSize; // top of the stack
            m_Fibers[i].IsFree = true;
            m_Fibers[i].Context = Context::Create(ptr, NormalStackSize, fiberCallback);
            Fmt::FormatTo(m_Fibers[i].Name, "Fiber {}", i);
        }
        for (uint32_t i = 0; i < ExtendedFiberCount; ++i)
        {
            Memory::ProtectVirtual(ptr, memorySpec.PageSize, Memory::ProtectFlags::None);
            ptr += memorySpec.PageSize + ExtendedStackSize;
            m_Fibers[i + NormalFiberCount].IsFree = true;
            m_Fibers[i + NormalFiberCount].Context = Context::Create(ptr, ExtendedStackSize, fiberCallback);
            Fmt::FormatTo(m_Fibers[i + NormalFiberCount].Name, "Fiber Big {}", i);
        }

        Memory::ProtectVirtual(ptr, memorySpec.PageSize, Memory::ProtectFlags::None);
        FE_Assert(ptr + memorySpec.PageSize == static_cast<std::byte*>(m_pStackMemory) + totalGuardPagesSize + TotalStackSize);
    }


    FiberPool::~FiberPool()
    {
        Memory::DefaultFree(m_Fibers);
    }


    FiberHandle FiberPool::Rent(bool extended)
    {
        const uint32_t fiberCount = extended ? ExtendedFiberCount : NormalFiberCount;
        const uint32_t baseFiberIndex = extended ? NormalFiberCount : 0;
        const uint32_t indexSeed = m_IndexSeed.fetch_add(1, std::memory_order_release);
        const uint64_t indexOffset = DefaultHash(&indexSeed, sizeof(indexSeed));

        while (true)
        {
            for (uint32_t fiberIndex = 0; fiberIndex < fiberCount; ++fiberIndex)
            {
                const uint32_t realIndex = static_cast<uint32_t>((fiberIndex + indexOffset) % fiberCount);
                FiberInfo& info = m_Fibers[realIndex + baseFiberIndex];
                if (!info.IsFree.load(std::memory_order_relaxed))
                    continue;

                bool expected = true;
                if (info.IsFree.compare_exchange_weak(expected, false, std::memory_order_release))
                    return FiberHandle{ realIndex + baseFiberIndex };
            }

            for (uint32_t spin = 0; spin < 32; ++spin)
                _mm_pause();
        }
    }


    void FiberPool::Return(FiberHandle fiberHandle)
    {
        m_Fibers[fiberHandle.Value].IsFree.store(true, std::memory_order_release);
    }


    void FiberPool::Update(FiberHandle fiberHandle, Context::Handle context)
    {
        if (fiberHandle)
            m_Fibers[fiberHandle.Value].Context = context;
    }


    Context::TransferParams FiberPool::Switch(FiberHandle to, uintptr_t userData)
    {
        TracyFiberEnter(m_Fibers[to.Value].Name.Data());
        return Context::Switch(m_Fibers[to.Value].Context, userData);
    }
} // namespace FE::Threading
