#pragma once
#include <Core/Math/UUID.h>
#include <Core/RTTI/RTTI.h>
#include <festd/string.h>
#include <festd/vector.h>

namespace FE::Serialization::Tests
{
    struct PackedDesc final
    {
        uint32_t m_width : 10 = 0;
        uint32_t m_height : 10 = 0;
        uint32_t m_flags : 12 = 0;
        float m_scale = 0.0f;

        FE_RTTI_Reflect("8E8F777D-28C6-47F7-89C6-E66E68A2B31D");
        FE_RTTI_Serialize();
    };


    struct TestObject final
    {
        static constexpr uint32_t kVersion = 1 + 2;

        Uuid m_id;
        PackedDesc m_desc;
        festd::vector<uint32_t> m_values;
        festd::array<float, 3> m_coordinates{};
        festd::string m_name;
        uint32_t m_transient FE_META(SkipSerializing) = 0;

        FE_RTTI("CC51822C-7386-4596-A208-F4A4950D63CB");
        FE_RTTI_Reflect();
        FE_RTTI_Serialize();
    };
} // namespace FE::Serialization::Tests
