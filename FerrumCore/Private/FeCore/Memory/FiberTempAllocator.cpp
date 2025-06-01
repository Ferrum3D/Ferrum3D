#include <FeCore/Memory/FiberTempAllocator.h>
#include <FeCore/Threading/Fiber.h>

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
        m_allocator->Restore(m_marker);
        m_allocator->Collect();
    }


    void* FiberTempAllocator::do_allocate(const size_t bytes, const size_t alignment)
    {
        return m_allocator->do_allocate(bytes, alignment);
    }
} // namespace FE::Memory
