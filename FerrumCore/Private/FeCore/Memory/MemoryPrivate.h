#pragma once
#include <memory_resource>

namespace FE::Memory::Internal
{
    void Init(std::pmr::memory_resource* allocator);
    void Shutdown();
} // namespace FE::Memory::Internal
