#pragma once
#include <EASTL/bonus/fixed_ring_buffer.h>
#include <EASTL/bonus/ring_buffer.h>
#include <festd/vector.h>

namespace FE::festd
{
    using eastl::fixed_ring_buffer;
    using eastl::ring_buffer;

    template<class T, size_t TCapacity>
    using inline_ring_buffer = ring_buffer<T, eastl::fixed_vector<T, TCapacity>>;
} // namespace FE::festd
