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

namespace FE::Internal
{
    //! Reports a failed EASTL assertion like FE_Assert does and breaks into the debugger.
    void ReportEASTLAssertion(const char* message, const char* fileName, int lineNumber);
} // namespace FE::Internal

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
// EASTL uses these in expressions as well as in statements.
#define EASTL_ASSERT(expr)                                                                                                       \
    ((expr) ? (void)0 : ::FE::Internal::ReportEASTLAssertion("Assertion Failed: " #expr, __FILE__, __LINE__))
#define EASTL_ASSERT_MSG(expr, msg) ((expr) ? (void)0 : ::FE::Internal::ReportEASTLAssertion(msg, __FILE__, __LINE__))
#define EASTL_FAIL_MSG(msg) ::FE::Internal::ReportEASTLAssertion(msg, __FILE__, __LINE__)
