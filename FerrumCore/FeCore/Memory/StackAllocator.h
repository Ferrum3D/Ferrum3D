#pragma once
#include <FeCore/Console/FeLog.h>
#include <FeCore/Base/Base.h>

namespace FE
{
    inline constexpr UInt32 FeStackAllocatorAlign = 16;

    class FeStackAllocator
    {
        UInt8* m_Memory{};
        size_t m_MemorySize{};
        size_t m_Allocated{};

    public:
        inline FeStackAllocator(size_t size)
        {
            Resize(size);
        }

        inline void Resize(size_t size)
        {
            FE_CORE_ASSERT(m_Allocated == 0, "Couldn't resize a non-empty allocator");
            size         = AlignUp<FeStackAllocatorAlign>(size);
            m_MemorySize = size;
            if (m_Memory)
                delete m_Memory;
            m_Memory = new UInt8[size];
        }

        inline void Reset()
        {
            m_Allocated = 0;
        }

        inline void* Malloc(size_t size)
        {
            UInt8* ptr = m_Memory + m_Allocated;
            m_Allocated += size;
            FE_CORE_ASSERT(m_Allocated <= m_MemorySize, "Allocator overflow");
            return ptr;
        }

        template<class T, class... Args>
        inline T* Allocate(Args&&... args)
        {
            return new (Malloc(sizeof(T))) T(args...);
        }

        template<class T>
        inline T* AllocateArray(size_t length)
        {
            return static_cast<T*>(Malloc(length * sizeof(T)));
        }
    };
} // namespace FE
