#pragma once
#include <memory_resource>

namespace FE::Threading::Internal
{
    void Init(std::pmr::memory_resource* allocator);
    void Shutdown();
} // namespace FE::Threading::Internal
