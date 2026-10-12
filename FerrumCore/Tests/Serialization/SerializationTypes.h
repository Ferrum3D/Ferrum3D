#pragma once
#include <Core/Math/UUID.h>
#include <Core/RTTI/RTTI.h>
#include <festd/string.h>
#include <festd/vector.h>

namespace FE::Serialization::Tests
{
    struct RuntimeBase
    {
        FE_RTTI("2339A6D0-0ECA-4898-A5B7-85BB8B841320");
        FE_RTTI_Reflect();

        uint32_t m_runtimeValue = 7;
    };


    struct SerializedBase
    {
        FE_RTTI("DC648E8E-6544-4637-95FB-8720C5E0F8A3");
        FE_RTTI_Reflect();
        FE_RTTI_Serialize();

        uint32_t m_baseValue = 0;
    };


    struct ManualSerializedBase
    {
        FE_RTTI("113937CB-7D40-4F5D-823D-7B535B4312B1");

        ResultCode Serialize(SerializationContext& context) const;
        ResultCode Deserialize(DeserializationContext& context);

        uint32_t m_manualValue = 0;
    };


    struct DerivedObject final
        : RuntimeBase
        , SerializedBase
        , ManualSerializedBase
    {
        FE_RTTI("D50860C5-2188-486F-8D18-A79D8C633C0C");
        FE_RTTI_Reflect();
        FE_RTTI_Serialize();

        uint32_t m_derivedValue = 0;
    };


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
        FE_SKIP_SERIALIZING uint32_t m_transient = 0;

        FE_RTTI("CC51822C-7386-4596-A208-F4A4950D63CB");
        FE_RTTI_Reflect();
        FE_RTTI_Serialize();
    };
} // namespace FE::Serialization::Tests
