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
        inline constexpr size_t kTotalStackSize = (kExtendedFiberCount + kNormalFiberCount) * kExtendedStackSize;

        inline constexpr uint32_t kFiberTempAllocatorPageSize = 64 * 1024;
    } // namespace


    FiberRuntimeInfo& FiberRuntimeInfo::Get()
    {
        //
        // To get the current fiber runtime info we need to get a pointer to the block of memory
        // allocated for the fiber's stack.
        // Since these blocks of memory are aligned to kExtendedStackSize, we can use a pointer
        // located somewhere on the stack and align it down to the start of the block.
        //

        auto* addressOnStack = FE_StackAlloc(std::byte, 0);
        auto* fiberStackStart = AlignDownPtr(addressOnStack, kExtendedStackSize);
        auto* result = reinterpret_cast<FiberRuntimeInfo*>(fiberStackStart);

        if (result->m_magic != kFiberMagic)
            FE_DebugBreak();

        return *result;
    }


    FiberPool::FiberPool(const Context::Callback fiberCallback)
    {
        FE_PROFILER_ZONE();

        m_tempPagePool.Initialize("FiberTempMemoryPool", kFiberTempAllocatorPageSize, kFiberTempAllocatorPageSize * 4);

        const Memory::PlatformSpec memorySpec = Memory::GetPlatformSpec();

        m_stackMemorySize = kTotalStackSize + kExtendedStackSize * 2;
        m_stackMemory = Memory::ReserveVirtual(m_stackMemorySize, kExtendedStackSize);

        m_fibers = Memory::DefaultAllocateArray<FiberInfo>(kTotalFiberCount);
        eastl::uninitialized_value_construct(m_fibers, m_fibers + kTotalFiberCount);

        auto* ptr = static_cast<std::byte*>(m_stackMemory);

        // First guard page.
        ptr += kExtendedStackSize;

        for (uint32_t i = 0; i < kNormalFiberCount; ++i)
        {
            Memory::CommitVirtual(ptr, kNormalStackSize - memorySpec.m_pageSize);

            auto* runtimeInfo = new (ptr) FiberRuntimeInfo(kFiberTempAllocatorPageSize, &m_tempPagePool);
            runtimeInfo->m_handle = FiberHandle{ i };
            FE_Assert(IsAlignedPtr(runtimeInfo, kExtendedStackSize));

            ptr += kNormalStackSize - memorySpec.m_pageSize; // top of the stack, last page is reserved

            const size_t stackByteSize = kNormalStackSize - memorySpec.m_pageSize - sizeof(FiberRuntimeInfo);
            m_fibers[i].m_runtimeInfo = runtimeInfo;
            m_fibers[i].m_context = Context::Create(ptr, stackByteSize, fiberCallback);
            m_fibers[i].m_isFree = true;
            Fmt::FormatTo(m_fibers[i].m_name, "Fiber {}", i);
            runtimeInfo->m_name = m_fibers[i].m_name.c_str();

            ptr = AlignUpPtr(ptr, kExtendedStackSize);
        }
        for (uint32_t i = 0; i < kExtendedFiberCount; ++i)
        {
            Memory::CommitVirtual(ptr, kExtendedStackSize - memorySpec.m_pageSize);

            auto* runtimeInfo = new (ptr) FiberRuntimeInfo(kFiberTempAllocatorPageSize, &m_tempPagePool);
            runtimeInfo->m_handle = FiberHandle{ i };
            FE_Assert(IsAlignedPtr(runtimeInfo, kExtendedStackSize));

            ptr += kExtendedStackSize - memorySpec.m_pageSize; // top of the stack, last page is reserved

            const size_t stackByteSize = kExtendedStackSize - memorySpec.m_pageSize - sizeof(FiberRuntimeInfo);
            m_fibers[i + kNormalFiberCount].m_runtimeInfo = runtimeInfo;
            m_fibers[i + kNormalFiberCount].m_context = Context::Create(ptr, stackByteSize, fiberCallback);
            m_fibers[i + kNormalFiberCount].m_isFree = true;
            Fmt::FormatTo(m_fibers[i + kNormalFiberCount].m_name, "Fiber Big {}", i);
            runtimeInfo->m_name = m_fibers[i + kNormalFiberCount].m_name.c_str();

            ptr = AlignUpPtr(ptr, kExtendedStackSize);
        }

        FE_Assert(ptr + kExtendedStackSize == static_cast<std::byte*>(m_stackMemory) + m_stackMemorySize);
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
        FE_Assert(m_fibers[fiberHandle.m_value].m_runtimeInfo->m_tempAllocator.IsEmpty());
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
