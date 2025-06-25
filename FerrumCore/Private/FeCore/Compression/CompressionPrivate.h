#pragma once
#include <memory_resource>

namespace FE::Compression::Internal
{
    void Init(std::pmr::memory_resource* allocator);
    void Shutdown();
} // namespace FE::Compression::Internal
