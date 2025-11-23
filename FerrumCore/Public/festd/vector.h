#pragma once
#include <EASTL/fixed_vector.h>
#include <EASTL/vector.h>
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

        template<typename T>
        struct TinyPolymorphicAllocator : std::pmr::polymorphic_allocator<T>
        {
            using size_type = uint32_t;
            using std::pmr::polymorphic_allocator<T>::polymorphic_allocator;
        };
    } // namespace Internal


    namespace pmr
    {
        template<class T>
        using vector = eastl::vector<T, Memory::Internal::EASTLPolymorphicAllocator>;

        template<class T, size_t TInlineCapacity = gch::default_buffer_size_v<Internal::TinyPolymorphicAllocator<T>>>
        using inline_vector = gch::small_vector<T, TInlineCapacity, Internal::TinyPolymorphicAllocator<T>>;
    } // namespace pmr


    template<class T, uint32_t TSize>
    using fixed_vector = eastl::fixed_vector<T, TSize, false>;

    using eastl::vector;


    template<class T, size_t TInlineCapacity = gch::default_buffer_size_v<Memory::StdDefaultAllocator<T, uint32_t>>>
    using inline_vector = gch::small_vector<T, TInlineCapacity, Memory::StdDefaultAllocator<T, uint32_t>>;
} // namespace FE::festd
