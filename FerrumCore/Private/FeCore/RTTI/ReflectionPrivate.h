#pragma once
#include <memory_resource>

namespace FE::Rtti::TypeRegistry::Internal
{
    void Init(std::pmr::memory_resource* allocator);
    void Shutdown();
} // namespace FE::Rtti::TypeRegistry::Internal
