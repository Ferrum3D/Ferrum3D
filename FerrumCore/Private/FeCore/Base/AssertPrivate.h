#pragma once
#include <memory_resource>

namespace FE::Trace::Internal
{
    void Init(std::pmr::memory_resource* allocator);
    void Shutdown();
} // namespace FE::Trace::Internal
