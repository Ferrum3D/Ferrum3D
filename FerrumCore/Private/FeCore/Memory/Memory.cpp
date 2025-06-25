#include <FeCore/Base/PlatformInclude.h>
#include <FeCore/Base/StackTrace.h>
#include <FeCore/Console/Console.h>
#include <FeCore/Memory/Memory.h>
#include <FeCore/Memory/MemoryPrivate.h>
#include <FeCore/Memory/tlsf.h>
#include <FeCore/Strings/Format.h>
#include <FeCore/Threading/SpinLock.h>

#include <DebugHeap.h>
#include <mimalloc.h>

namespace FE::Memory
{
    namespace
    {
        struct MemoryState final
        {
            Threading::SpinLock m_lock;
            DebugHeap* m_debugHeap = nullptr;

            MemoryState()
            {
                constexpr size_t kGigabyte = 0x40000000;
                if (Build::IsDevelopment())
                    m_debugHeap = DebugHeapInit(8 * kGigabyte);
            }

            ~MemoryState()
            {
                if (m_debugHeap)
                {
                    DebugHeapWalk(
                        m_debugHeap,
                        [](void* ptr, const size_t size, const uint32_t callstackHandle, void*) {
                            const Trace::CallStack callstack{ callstackHandle };

                            Console::SetTextColor(Console::Color::kRed);
                            Console::Write(
                                Fmt::FixedFormat("Memory leak detected: {}; {} bytes\n", reinterpret_cast<uintptr_t>(ptr), size));

                            void** frames = callstack.GetFrames();
                            for (uint32_t frameIndex = 0; frames[frameIndex] != nullptr; ++frameIndex)
                            {
                                const Trace::SymbolInfo symbolInfo = Trace::SymbolInfo::Resolve(frames[frameIndex]);
                                Console::Write(Fmt::FixedFormatSized<512>("{}\t[{}]\t{}:{}: {}\n",
                                                                          symbolInfo.m_moduleName,
                                                                          symbolInfo.m_address,
                                                                          symbolInfo.m_fileName,
                                                                          symbolInfo.m_lineNumber,
                                                                          symbolInfo.m_symbolName));
                            }

                            Console::SetTextColor(Console::Color::kDefault);
                        },
                        nullptr);
                    DebugHeapDestroy(m_debugHeap);
                }
            }
        };

        MemoryState* GMemoryState;


        constexpr uint32_t kMemoryProfilerCallstackDepth = 32;

#if FE_PLATFORM_WINDOWS
        decltype(&VirtualAlloc2) GetVirtualAlloc2()
        {
            static auto virtualAlloc2 =
                reinterpret_cast<decltype(&VirtualAlloc2)>(GetProcAddress(GetModuleHandleW(L"kernelbase.dll"), "VirtualAlloc2"));
            return virtualAlloc2;
        }
#endif

        void* MallocImpl(const size_t byteSize)
        {
            if (Build::IsDevelopment())
            {
                if (GMemoryState->m_debugHeap)
                {
                    std::lock_guard lock{ GMemoryState->m_lock };
                    const Trace::CallStack callstack = Trace::CallStack::Capture(32, Trace::CallStack::kDefaultSkipFrames + 2);
                    return DebugHeapAllocate(GMemoryState->m_debugHeap, byteSize, kDefaultAlignment, callstack.m_value);
                }
            }

            return mi_malloc(byteSize);
        }

        void* MallocAlignedImpl(const size_t byteSize, const size_t byteAlignment)
        {
            if (Build::IsDevelopment())
            {
                if (GMemoryState->m_debugHeap)
                {
                    std::lock_guard lock{ GMemoryState->m_lock };
                    const Trace::CallStack callstack = Trace::CallStack::Capture(32, Trace::CallStack::kDefaultSkipFrames + 2);
                    return DebugHeapAllocate(GMemoryState->m_debugHeap, byteSize, byteAlignment, callstack.m_value);
                }
            }

            return mi_malloc_aligned(byteSize, byteAlignment);
        }

        void* ReallocImpl(void* ptr, const size_t byteSize)
        {
            if (Build::IsDevelopment())
            {
                if (GMemoryState->m_debugHeap)
                {
                    std::lock_guard lock{ GMemoryState->m_lock };
                    if (byteSize == 0)
                    {
                        if (ptr != nullptr)
                            DebugHeapFree(GMemoryState->m_debugHeap, ptr);

                        return nullptr;
                    }

                    const Trace::CallStack callstack = Trace::CallStack::Capture(32, Trace::CallStack::kDefaultSkipFrames + 2);

                    if (ptr == nullptr)
                        return DebugHeapAllocate(GMemoryState->m_debugHeap, byteSize, kDefaultAlignment, callstack.m_value);

                    const size_t oldSize = DebugHeapGetAllocSize(GMemoryState->m_debugHeap, ptr);
                    void* newPtr = DebugHeapAllocate(GMemoryState->m_debugHeap, byteSize, kDefaultAlignment, callstack.m_value);
                    memcpy(newPtr, ptr, Math::Min(oldSize, byteSize));
                    DebugHeapFree(GMemoryState->m_debugHeap, ptr);
                    return newPtr;
                }
            }

            return mi_realloc(ptr, byteSize);
        }

        void FreeImpl(void* ptr)
        {
            if (Build::IsDevelopment())
            {
                if (GMemoryState->m_debugHeap)
                {
                    if (ptr == nullptr)
                        return;

                    std::lock_guard lock{ GMemoryState->m_lock };
                    DebugHeapFree(GMemoryState->m_debugHeap, ptr);
                    return;
                }
            }

            mi_free(ptr);
        }

        size_t GetAllocatedSizeImpl(const void* ptr)
        {
            if (Build::IsDevelopment())
            {
                if (GMemoryState->m_debugHeap)
                {
                    std::unique_lock lock{ GMemoryState->m_lock };
                    return DebugHeapGetAllocSize(GMemoryState->m_debugHeap, const_cast<void*>(ptr));
                }
            }

            return mi_usable_size(ptr);
        }

        void AssertPointerIsValidImpl(const void* ptr)
        {
            if (!Build::IsDevelopment())
                return;

            if (!GMemoryState->m_debugHeap)
                return;

            std::unique_lock lock{ GMemoryState->m_lock };

            const Trace::CallStack callstack{ DebugHeapGetCallstackIfInvalid(GMemoryState->m_debugHeap, const_cast<void*>(ptr)) };

            if (callstack.IsValid())
            {
                void** frames = callstack.GetFrames();
                for (uint32_t frameIndex = 0; frames[frameIndex] != nullptr; ++frameIndex)
                {
                    const Trace::SymbolInfo symbolInfo = Trace::SymbolInfo::Resolve(frames[frameIndex]);
                    Console::Write(Fmt::FixedFormatSized<512>("{}\t[{}]\t{}:{}: {}\n",
                                                              symbolInfo.m_moduleName,
                                                              symbolInfo.m_address,
                                                              symbolInfo.m_fileName,
                                                              symbolInfo.m_lineNumber,
                                                              symbolInfo.m_symbolName));
                }

                // Flush might allocate memory, so we need to unlock the heap lock.
                lock.unlock();
                Console::Flush();
                FE_DebugBreak();
            }
        }
    } // namespace


    void Internal::Init(std::pmr::memory_resource* allocator)
    {
        FE_CoreAssert(GMemoryState == nullptr, "Memory already initialized");
        GMemoryState = Memory::New<MemoryState>(allocator);
    }


    void Internal::Shutdown()
    {
        FE_CoreAssert(GMemoryState != nullptr, "Memory not initialized");
        GMemoryState->~MemoryState();
        GMemoryState = nullptr;
    }


    PlatformSpec GetPlatformSpec()
    {
        static PlatformSpec cachedSpec = [] {
            PlatformSpec spec = {};

#if FE_PLATFORM_WINDOWS
            SYSTEM_INFO info;
            GetSystemInfo(&info);

            spec.m_pageSize = info.dwPageSize;
            spec.m_granularity = info.dwAllocationGranularity;
#else
#    error Not implemented :(
#endif
            return spec;
        }();

        return cachedSpec;
    }


    void* AllocateVirtual(const size_t byteSize)
    {
        FE_CoreAssert(IsAligned(byteSize, GetPlatformSpec().m_granularity),
                      "Size must be a multiple of virtual allocation granularity");

#if FE_PLATFORM_WINDOWS
        return VirtualAlloc(nullptr, byteSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#else
#    error Not implemented :(
#endif
    }


    void* AllocateVirtual(const size_t byteSize, const size_t byteAlignment)
    {
        FE_CoreAssert(IsAligned(byteSize, byteAlignment), "Size must be a multiple of alignment");
        FE_CoreAssert(IsAligned(byteAlignment, GetPlatformSpec().m_granularity),
                      "Alignment must be a multiple of virtual allocation granularity");

#if FE_PLATFORM_WINDOWS
        MEM_ADDRESS_REQUIREMENTS requirements = {};
        requirements.Alignment = byteAlignment;

        MEM_EXTENDED_PARAMETER extendedParameter = {};
        extendedParameter.Type = MemExtendedParameterAddressRequirements;
        extendedParameter.Pointer = &requirements;

        return GetVirtualAlloc2()(
            GetCurrentProcess(), nullptr, byteSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE, &extendedParameter, 1);
#else
#    error Not implemented :(
#endif
    }


    void* ReserveVirtual(const size_t byteSize)
    {
#if FE_PLATFORM_WINDOWS
        return VirtualAlloc(nullptr, byteSize, MEM_RESERVE, PAGE_READWRITE);
#else
#    error Not implemented :(
#endif
    }


    void* ReserveVirtual(const size_t byteSize, const size_t byteAlignment)
    {
        FE_CoreAssert(IsAligned(byteSize, byteAlignment), "Size must be a multiple of alignment");
        FE_CoreAssert(IsAligned(byteAlignment, GetPlatformSpec().m_granularity),
                      "Alignment must be a multiple of virtual allocation granularity");

#if FE_PLATFORM_WINDOWS
        MEM_ADDRESS_REQUIREMENTS requirements = {};
        requirements.Alignment = byteAlignment;

        MEM_EXTENDED_PARAMETER extendedParameter = {};
        extendedParameter.Type = MemExtendedParameterAddressRequirements;
        extendedParameter.Pointer = &requirements;

        return GetVirtualAlloc2()(GetCurrentProcess(), nullptr, byteSize, MEM_RESERVE, PAGE_READWRITE, &extendedParameter, 1);
#else
#    error Not implemented :(
#endif
    }


    void CommitVirtual(void* ptr, const size_t byteSize)
    {
#if FE_PLATFORM_WINDOWS
        VirtualAlloc(ptr, byteSize, MEM_COMMIT, PAGE_READWRITE);
#else
#    error Not implemented :(
#endif
    }


    void FreeVirtual(void* ptr, [[maybe_unused]] const size_t byteSize)
    {
#if FE_PLATFORM_WINDOWS
        VirtualFree(ptr, 0, MEM_RELEASE);
#else
#    error Not implemented :(
#endif
    }


    void ProtectVirtual(void* ptr, const size_t byteSize, const ProtectFlags protection)
    {
#if FE_PLATFORM_WINDOWS
        DWORD osProtect = 0;
        switch (protection)
        {
        case ProtectFlags::kNone:
            osProtect = PAGE_NOACCESS;
            break;
        case ProtectFlags::kReadOnly:
            osProtect = PAGE_READONLY;
            break;
        case ProtectFlags::kReadWrite:
            osProtect = PAGE_READWRITE;
            break;
        default:
            FE_DebugBreak();
            break;
        }

        DWORD oldProtect = 0;
        const BOOL result = VirtualProtect(ptr, byteSize, osProtect, &oldProtect);
        FE_CoreAssert(result);
#else
#    error Not implemented :(
#endif
    }


    void* DefaultAllocate(const size_t byteSize)
    {
        void* ptr = MallocImpl(byteSize);
        TracySecureAllocS(ptr, byteSize, kMemoryProfilerCallstackDepth);
        return ptr;
    }


    void* DefaultAllocate(const size_t byteSize, const size_t byteAlignment)
    {
        void* ptr = MallocAlignedImpl(byteSize, byteAlignment);
        TracySecureAllocS(ptr, byteSize, kMemoryProfilerCallstackDepth);
        return ptr;
    }


    void* DefaultReallocate(void* ptr, const size_t newSize)
    {
        void* newPtr = ReallocImpl(ptr, newSize);
        TracySecureFreeS(ptr, kMemoryProfilerCallstackDepth);
        TracySecureAllocS(newPtr, newSize, kMemoryProfilerCallstackDepth);
        return newPtr;
    }


    void DefaultFree(void* ptr)
    {
        TracySecureFreeS(ptr, kMemoryProfilerCallstackDepth);
        FreeImpl(ptr);
    }


    size_t GetAllocatedSize(const void* ptr)
    {
        return GetAllocatedSizeImpl(ptr);
    }


    void AssertPointerIsValid(const void* ptr)
    {
        AssertPointerIsValidImpl(ptr);
    }


    TLSFAllocator::TLSFAllocator(void* memory, const size_t size)
    {
        if (memory == nullptr)
        {
            memory = AllocateVirtual(size);
            m_ownedMemory = memory;
        }

        m_size = size;
        m_impl = tlsf_create_with_pool(memory, size);
    }


    TLSFAllocator::~TLSFAllocator()
    {
        tlsf_destroy(m_impl);

        if (m_ownedMemory != nullptr)
            FreeVirtual(m_ownedMemory, m_size);
    }


    void* TLSFAllocator::do_allocate(const size_t byteSize, const size_t byteAlignment)
    {
        return tlsf_memalign(m_impl, byteAlignment, byteSize);
    }


    void TLSFAllocator::do_deallocate(void* ptr, const size_t byteSize, const size_t byteAlignment)
    {
        FE_Unused(byteSize);
        FE_Unused(byteAlignment);
        tlsf_free(m_impl, ptr);
    }
} // namespace FE::Memory
