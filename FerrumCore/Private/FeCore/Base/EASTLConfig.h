#pragma once
#include <cassert>
#include <cstddef>
#include <cstdint>

namespace FE::Memory
{
    void* DefaultAllocate(size_t byteSize);
    void* DefaultAllocate(size_t byteSize, size_t byteAlignment);
    void DefaultFree(void* ptr);
} // namespace FE::Memory

namespace FE::Memory::Internal
{
    struct EASTLDefaultAllocator
    {
        constexpr EASTLDefaultAllocator() = default;
        constexpr EASTLDefaultAllocator(const char*) {}

        inline void* allocate(size_t n, int = 0)
        {
            return DefaultAllocate(n);
        }

        inline void* allocate(size_t n, size_t alignment, size_t, int = 0)
        {
            return DefaultAllocate(n, alignment);
        }

        inline void deallocate(void* p, size_t)
        {
            DefaultFree(p);
        }

        inline const char* get_name() const
        {
            return "";
        }

        inline void set_name(const char*) {}

        friend bool operator==(const EASTLDefaultAllocator&, const EASTLDefaultAllocator&)
        {
            return true;
        }

        friend bool operator!=(const EASTLDefaultAllocator&, const EASTLDefaultAllocator&)
        {
            return false;
        }
    };


    inline static EASTLDefaultAllocator* GetDefaultEASTLAllocator()
    {
        static EASTLDefaultAllocator allocator;
        return &allocator;
    }
} // namespace FE::Memory::Internal


#define EASTL_NAME_ENABLED 0
#define EASTL_RTTI_ENABLED 0
#define EASTL_EXCEPTIONS_ENABLED 0
#define EASTL_SIZE_T_32BIT 1
#define EASTLAllocatorType ::FE::Memory::Internal::EASTLDefaultAllocator
#define EASTLAllocatorDefault ::FE::Memory::Internal::GetDefaultEASTLAllocator
#define EASTL_ASSERT assert
#define EASTL_ASSERT_MSG(expr, msg) assert((expr) && (msg))
#define EASTL_FAIL_MSG(msg) assert(0 && (msg))
