#include <FeCore/Memory/AllocatorBase.h>

namespace FE
{
    AllocatorBase::AllocatorBase(const char* name, const char* description) noexcept
        : m_Name(name)
        , m_Description(description)
        , m_Initialized(false)
    {
    }

    const char* AllocatorBase::GetName() const noexcept
    {
        return m_Name;
    }

    const char* AllocatorBase::GetDescription() const noexcept
    {
        return m_Description;
    }

    bool AllocatorBase::Initialized() const noexcept
    {
        return m_Initialized;
    }

    IAllocator* AllocatorBase::Get() noexcept
    {
        return this;
    }

    void AllocatorBase::CollectGarbage() {}

    size_t AllocatorBase::MaxBlockSize() const
    {
        return size_t(-1);
    }

    void AllocatorBase::CopyMemory(void* destination, void* source, size_t byteSize) const
    {
        memcpy(destination, source, byteSize);
    }

    void AllocatorBase::ProfileAllocate(size_t size, size_t alignment, const SourcePosition& position)
    {
        (void)size;
        (void)alignment;
        (void)position;
        // TODO: in-engine memory profiler
    }

    void AllocatorBase::ProfileDeallocate(void* pointer, const SourcePosition& position, size_t size)
    {
        (void)position;
        if (size == 0)
            size = SizeOfBlock(pointer);
        // TODO: in-engine memory profiler
    }

    void AllocatorBase::ProfileReallocate(
        void* pointer, const SourcePosition& position, size_t newSize, size_t newAlignment, size_t oldSize)
    {
        (void)position;
        (void)newSize;
        (void)newAlignment;
        if (oldSize == 0)
            oldSize = SizeOfBlock(pointer);
        // TODO: in-engine memory profiler
    }
} // namespace FE
