#pragma once
#include <FeCore/Memory/Memory.h>
#include <ankerl/unordered_dense.h>

namespace FE::festd
{
    // TODO: change the hasher
    template<class TKey, class TValue, class THash = eastl::hash<TKey>, class TKeyEqual = std::equal_to<TKey>>
    using unordered_dense_map =
        ankerl::unordered_dense::map<TKey, TValue, THash, TKeyEqual, Memory::StdDefaultAllocator<std::pair<TKey, TValue>>>;

    template<class TKey, class TValue, class THash = eastl::hash<TKey>, class TKeyEqual = std::equal_to<TKey>>
    using segmented_unordered_dense_map =
        ankerl::unordered_dense::segmented_map<TKey, TValue, THash, TKeyEqual,
                                               Memory::StdDefaultAllocator<std::pair<TKey, TValue>>>;

    template<class TKey, class THash = eastl::hash<TKey>, class TKeyEqual = std::equal_to<TKey>>
    using unordered_dense_set = ankerl::unordered_dense::set<TKey, THash, TKeyEqual, Memory::StdDefaultAllocator<TKey>>;

    template<class TKey, class THash = eastl::hash<TKey>, class TKeyEqual = std::equal_to<TKey>>
    using segmented_unordered_dense_set =
        ankerl::unordered_dense::segmented_set<TKey, THash, TKeyEqual, Memory::StdDefaultAllocator<TKey>>;

    namespace pmr
    {
        template<class TKey, class TValue, class THash = eastl::hash<TKey>, class TKeyEqual = std::equal_to<TKey>>
        using unordered_dense_map = ankerl::unordered_dense::pmr::map<TKey, TValue, THash, TKeyEqual>;

        template<class TKey, class TValue, class THash = eastl::hash<TKey>, class TKeyEqual = std::equal_to<TKey>>
        using segmented_unordered_dense_map = ankerl::unordered_dense::pmr::segmented_map<TKey, TValue, THash, TKeyEqual>;

        template<class TKey, class THash = eastl::hash<TKey>, class TKeyEqual = std::equal_to<TKey>>
        using unordered_dense_set = ankerl::unordered_dense::pmr::set<TKey, THash, TKeyEqual>;

        template<class TKey, class THash = eastl::hash<TKey>, class TKeyEqual = std::equal_to<TKey>>
        using segmented_unordered_dense_set = ankerl::unordered_dense::pmr::segmented_set<TKey, THash, TKeyEqual>;
    } // namespace pmr
} // namespace FE::festd
