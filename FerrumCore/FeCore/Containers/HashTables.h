#pragma once
#include <FeCore/Memory/Memory.h>
#include <ankerl/unordered_dense.h>

namespace FE::festd
{
    // TODO: change the hasher
    template<class TKey, class TValue, class THash = eastl::hash<TKey>, class TKeyEqual = std::equal_to<TKey>>
    using unordered_dense_map =
        ankerl::unordered_dense::map<TKey, TValue, THash, TKeyEqual, Memory::StdDefaultAllocator<std::pair<TKey, TValue>>>;
} // namespace FE::festd
