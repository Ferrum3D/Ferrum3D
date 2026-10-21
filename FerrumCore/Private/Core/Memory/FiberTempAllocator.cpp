#include <Core/Memory/FiberTempAllocator.h>
#include <Core/Threading/Fiber.h>

namespace FE::Memory
{
    FiberTempAllocator::FiberTempAllocator()
    {
        Threading::FiberRuntimeInfo& info = Threading::FiberRuntimeInfo::Get();
        m_allocator = &info.m_tempAllocator;
        m_marker = m_allocator->GetMarker();
    }


    FiberTempAllocator::~FiberTempAllocator()
    {
        while (m_oversized)
        {
            OversizedBlock* block = m_oversized;
            m_oversized = block->m_next;
            DefaultFree(block);
        }

        m_allocator->Restore(m_marker);
        m_allocator->FreeUnusedMemory();
    }


    void* FiberTempAllocator::do_allocate(const size_t bytes, const size_t alignment)
    {
        if (void* result = m_allocator->do_allocate(bytes, alignment))
            return result;

        // Prefix the block with a list header padded to the requested alignment.
        const size_t blockAlignment = Math::Max(alignment, alignof(OversizedBlock));
        const size_t headerSize = AlignUp(sizeof(OversizedBlock), blockAlignment);
        auto* block = static_cast<OversizedBlock*>(DefaultAllocate(headerSize + bytes, blockAlignment));
        block->m_next = m_oversized;
        m_oversized = block;
        return reinterpret_cast<std::byte*>(block) + headerSize;
    }
} // namespace FE::Memory
