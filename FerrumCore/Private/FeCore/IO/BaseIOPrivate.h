#pragma once
#include <memory_resource>

namespace FE::IO::Internal
{
    void Init(std::pmr::memory_resource* allocator);
    void Shutdown();
} // namespace FE::IO::Internal
