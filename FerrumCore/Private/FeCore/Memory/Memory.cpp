#include <FeCore/Base/PlatformInclude.h>
#include <FeCore/Memory/Memory.h>
#include <FeCore/Parallel/SpinLock.h>
#include <mimalloc.h>

namespace FE::Memory
{
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


    void* AllocateVirtual(size_t byteSize)
    {
        FE_CORE_ASSERT(AlignUp(byteSize, GetPlatformSpec().m_granularity) == byteSize,
                       "Size must be aligned to virtual allocation granularity");

#if FE_PLATFORM_WINDOWS
        return VirtualAlloc(nullptr, byteSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#else
#    error Not implemented :(
#endif
    }


    void FreeVirtual(void* ptr, size_t byteSize)
    {
#if FE_PLATFORM_WINDOWS
        (void)byteSize;
        VirtualFree(ptr, 0, MEM_RELEASE);
#else
#    error Not implemented :(
#endif
    }


    void ProtectVirtual(void* ptr, size_t byteSize, ProtectFlags protection)
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

    void* DefaultAllocate(size_t byteSize)
    {
        return mi_malloc(byteSize);
    }


    void* DefaultAllocate(size_t byteSize, size_t byteAlignment)
    {
        return mi_malloc_aligned(byteSize, byteAlignment);
    }


    void* DefaultReallocate(void* ptr, size_t newSize)
    {
        return mi_realloc(ptr, newSize);
    }


    void DefaultFree(void* ptr)
    {
        mi_free(ptr);
    }
} // namespace FE::Memory
