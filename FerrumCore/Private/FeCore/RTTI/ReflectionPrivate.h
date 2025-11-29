#pragma once
#include <memory_resource>

namespace FE::RTTI::TypeRegistry::Internal
{
    void Init(std::pmr::memory_resource* allocator);
    void Shutdown();
} // namespace FE::RTTI::TypeRegistry::Internal
