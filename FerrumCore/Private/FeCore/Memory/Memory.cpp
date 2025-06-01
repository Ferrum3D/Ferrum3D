#include <FeCore/Base/PlatformInclude.h>
#include <FeCore/Memory/Memory.h>
#include <FeCore/Memory/tlsf.h>
#include <FeCore/Threading/SpinLock.h>
#include <mimalloc.h>

namespace FE::Memory
{
    namespace
    {
        constexpr uint32_t kMemoryProfilerCallstackDepth = 32;

#if FE_PLATFORM_WINDOWS
        decltype(&VirtualAlloc2) GetVirtualAlloc2()
        {
            static auto virtualAlloc2 =
                reinterpret_cast<decltype(&VirtualAlloc2)>(GetProcAddress(GetModuleHandleW(L"kernelbase.dll"), "VirtualAlloc2"));
            return virtualAlloc2;
        }
#endif
    } // namespace


    PlatformSpec GetPlatformSpec()
    {
        static PlatformSpec s_result;
        if (s_result.m_pageSize != 0)
            return s_result;

        static Threading::SpinLock s_spinLock;
        std::lock_guard lk{ s_spinLock };
        if (s_result.m_pageSize != 0)
            return s_result;

#if FE_PLATFORM_WINDOWS
        SYSTEM_INFO info;
        GetSystemInfo(&info);
        s_result.m_pageSize = info.dwPageSize;
        s_result.m_granularity = info.dwAllocationGranularity;
#else
#    error Not implemented :(
#endif

        return s_result;
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
        (void)byteSize;
        VirtualAlloc(ptr, byteSize, MEM_COMMIT, PAGE_READWRITE);
#else
#    error Not implemented :(
#endif
    }


    void FreeVirtual(void* ptr, const size_t byteSize)
    {
#if FE_PLATFORM_WINDOWS
        (void)byteSize;
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
        assert(result);
#else
#    error Not implemented :(
#endif
    }


    void* DefaultAllocate(const size_t byteSize)
    {
        void* ptr = mi_malloc(byteSize);
        TracySecureAllocS(ptr, byteSize, kMemoryProfilerCallstackDepth);
        return ptr;
    }


    void* DefaultAllocate(const size_t byteSize, const size_t byteAlignment)
    {
        void* ptr = mi_malloc_aligned(byteSize, byteAlignment);
        TracySecureAllocS(ptr, byteSize, kMemoryProfilerCallstackDepth);
        return ptr;
    }


    void* DefaultReallocate(void* ptr, const size_t newSize)
    {
        void* newPtr = mi_realloc(ptr, newSize);
        TracySecureFreeS(ptr, kMemoryProfilerCallstackDepth);
        TracySecureAllocS(newPtr, newSize, kMemoryProfilerCallstackDepth);
        return newPtr;
    }


    void DefaultFree(void* ptr)
    {
        TracySecureFreeS(ptr, kMemoryProfilerCallstackDepth);
        mi_free(ptr);
    }


    TlsfAllocator::TlsfAllocator(void* memory, const size_t size)
    {
        if (memory == nullptr)
        {
            memory = AllocateVirtual(size);
            m_ownedMemory = memory;
        }

        m_size = size;
        m_impl = tlsf_create_with_pool(memory, size);
    }


    TlsfAllocator::~TlsfAllocator()
    {
        tlsf_destroy(m_impl);

        if (m_ownedMemory != nullptr)
            FreeVirtual(m_ownedMemory, m_size);
    }


    void* TlsfAllocator::do_allocate(const size_t byteSize, const size_t byteAlignment)
    {
        return tlsf_memalign(m_impl, byteAlignment, byteSize);
    }


    void TlsfAllocator::do_deallocate(void* ptr, size_t byteSize, size_t byteAlignment)
    {
        (void)byteSize;
        (void)byteAlignment;

        tlsf_free(m_impl, ptr);
    }
} // namespace FE::Memory
