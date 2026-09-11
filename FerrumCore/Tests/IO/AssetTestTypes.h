#pragma once
#include <Core/IO/Assets.h>

namespace FE::IO::Tests
{
    struct SyntheticAsset
    {
        uint32_t m_value = 0;
        Link<SyntheticAsset> m_dependency;

        FE_RTTI("D734D82A-4790-41E9-B428-2D639BE4CF17");
        FE_RTTI_Reflect();
        FE_RTTI_Serialize();
    };
} // namespace FE::IO::Tests
