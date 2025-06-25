#pragma once
#include <memory_resource>

namespace FE::Trace::Internal
{
    void InitStackTrace(std::pmr::memory_resource* allocator);
    void ShutdownStackTrace();
} // namespace FE::Trace::Internal
