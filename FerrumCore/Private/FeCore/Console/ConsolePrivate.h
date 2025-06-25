#pragma once
#include <memory_resource>

namespace FE::Console::Internal
{
    void Init(std::pmr::memory_resource* allocator);
    void Shutdown();
} // namespace FE::Console::Internal
