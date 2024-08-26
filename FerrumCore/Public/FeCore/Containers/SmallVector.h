#pragma once
#include <FeCore/Memory/Memory.h>
#include <gch/small_vector.hpp>

namespace FE::festd
{
    namespace Internal
    {
        template<typename T>
        struct TinyAllocator : std::allocator<T>
        {
            using size_type = uint32_t;
            using std::allocator<T>::allocator;
        };
    } // namespace Internal


#if __INTELLISENSE__
    template<class T, size_t TCapacity = gch::default_buffer_size_v<Internal::TinyAllocator<T>>>
    using small_vector = gch::small_vector<T, TCapacity, Internal::TinyAllocator<T>>;
#else
    template<class T, size_t TCapacity = gch::default_buffer_size_v<Memory::StdDefaultAllocator<T, uint32_t>>>
    using small_vector = gch::small_vector<T, TCapacity, Memory::StdDefaultAllocator<T, uint32_t>>;

    static_assert(sizeof(gch::small_vector<int, gch::default_buffer_size_v<Memory::StdDefaultAllocator<int, uint32_t>>,
                                           Memory::StdDefaultAllocator<int, uint32_t>>)
                      == sizeof(gch::small_vector<int, gch::default_buffer_size_v<Internal::TinyAllocator<int>>,
                                                  Internal::TinyAllocator<int>>),
                  "Intellisense doesn't report the correct size for small_vector");
#endif
} // namespace FE::festd
