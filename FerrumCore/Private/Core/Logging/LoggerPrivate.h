#pragma once
#include <memory_resource>

namespace FE::Logger
{
    struct SinkBase;

    namespace Internal
    {
        void Init(std::pmr::memory_resource* allocator);
        void Shutdown();
    } // namespace Internal
} // namespace FE::Logger
