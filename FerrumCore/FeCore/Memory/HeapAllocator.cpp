#include <FeCore/Memory/HeapAllocator.h>

#define RPMALLOC_CONFIGURABLE 1
#include <rpmalloc/rpmalloc.h>

#if 1
#    define FE_MALLOC_PRINT(...)                                                                                                 \
        do                                                                                                                       \
        {                                                                                                                        \
        }                                                                                                                        \
        while (0)
#else
#    include <cstdio>
#    define FE_MALLOC_PRINT(...)                                                                                                 \
        do                                                                                                                       \
        {                                                                                                                        \
            printf_s(__VA_ARGS__);                                                                                               \
            printf_s("\n\tAt file \"%s\"; line %i; caller %s\n\n", position.FileName, position.LineNumber, position.FuncName);   \
        }                                                                                                                        \
        while (0)
#endif

namespace FE
{
    HeapAllocator::HeapAllocator()
        : AllocatorBase("HeapAllocator", "Main generic heap allocator")
        , m_TotalUsage(0)
    {
    }

    HeapAllocator::~HeapAllocator() noexcept = default;

    void HeapAllocator::Init(const Desc& desc)
    {
        rpmalloc_config_t conf{};
        conf.page_size      = desc.PageSize;
        conf.span_size      = desc.SpanSize;
        conf.span_map_count = desc.SpanMapCount;
        rpmalloc_initialize_config(&conf);
        m_Initialized = true;
    }

    void* HeapAllocator::Allocate(USize size, USize alignment, [[maybe_unused]] const SourcePosition& position)
    {
        Interlocked::Add(m_TotalUsage, size);
        FE_PROFILE_ALLOC(size, alignment, position);
        void* r = rpaligned_alloc(alignment, size);
        FE_MALLOC_PRINT("Allocation: %llu bytes, returned: %p", size, r);
        return r;
    }

    void HeapAllocator::Deallocate(void* pointer, [[maybe_unused]] const SourcePosition& position, USize size)
    {
        if (size == 0)
            size = SizeOfBlock(pointer);
        Interlocked::Add(m_TotalUsage, -static_cast<SSize>(size));
        FE_PROFILE_DEALLOC(pointer, position, size);
        rpfree(pointer);
        FE_MALLOC_PRINT("Deallocation: %llu bytes at %p", size, pointer);
    }

    void* HeapAllocator::Reallocate(
        void* pointer, [[maybe_unused]] const SourcePosition& position, USize newSize, USize newAlignment, USize oldSize)
    {
        if (oldSize == 0)
            oldSize = SizeOfBlock(pointer);
        Interlocked::Add(m_TotalUsage, newSize - oldSize);
        FE_PROFILE_REALLOC(pointer, position, newSize, newAlignment, oldSize);
        return rpaligned_realloc(pointer, newAlignment, newSize, oldSize, 0);
    }

    USize HeapAllocator::TotalAllocated() const
    {
        return m_TotalUsage;
    }

    USize HeapAllocator::SizeOfBlock(void* pointer)
    {
        return rpmalloc_usable_size(pointer);
    }

    void HeapAllocator::ThreadInit()
    {
        rpmalloc_thread_initialize();
    }
} // namespace FE
