#pragma once
#include <Framework/Input/Action.h>
#include <Framework/Input/Core/Keys.h>
#include <festd/variant.h>

namespace FE::Input
{
    struct Control final
    {
        FE_RTTI_Reflect("26C1E4CC-F60E-4474-83EB-C892B0301E4D");
        FE_RTTI_Serialize();

        //! Keys use their native code; mouse buttons use 256..260. Zero is invalid.
        uint16_t m_code = 0;

        static Control Key(const Core::Key key)
        {
            return { static_cast<uint16_t>(key) };
        }

        static Control MouseButton(const uint32_t button)
        {
            return { static_cast<uint16_t>(256 + button) };
        }

        [[nodiscard]] bool Validate() const;
    };


    struct ButtonBinding final
    {
        FE_RTTI_Reflect("B539603C-9AC3-4724-B058-838B64070C25");
        FE_RTTI_Serialize();
        Control m_control;
    };


    struct Axis1DBinding final
    {
        FE_RTTI_Reflect("E38772C6-2BFF-40A2-ADF3-CB3AB513B182");
        FE_RTTI_Serialize();
        Control m_negative;
        Control m_positive;
    };


    struct Axis2DBinding final
    {
        FE_RTTI_Reflect("156E92A7-71EB-447C-85DB-DA3F6EB41B91");
        FE_RTTI_Serialize();
        Control m_left;
        Control m_right;
        Control m_back;
        Control m_forward;
    };


    struct MouseDeltaBinding final
    {
        FE_RTTI_Reflect("894F98CF-A08F-4708-A102-A66661243CEE");
        FE_RTTI_Serialize();
    };


    struct MouseWheelBinding final
    {
        FE_RTTI_Reflect("23675BB7-7F41-461F-9010-3310208EC616");
        FE_RTTI_Serialize();
    };


    struct ChordBinding final
    {
        FE_RTTI_Reflect("850C0C4B-A883-4FA1-B40E-1BBA4E113E12");
        FE_RTTI_Serialize();

        festd::vector<Control> m_controls;
    };


    struct Binding final
    {
        FE_RTTI_Reflect("AF710BC7-D755-4116-A5DC-F44202849DFC");
        FE_RTTI_Serialize();

        festd::variant<ButtonBinding, Axis1DBinding, Axis2DBinding, MouseDeltaBinding, MouseWheelBinding, ChordBinding> m_source;
        festd::vector<Control> m_conditions;
        Vector2 m_scale{ 1, 1 };
        float m_sensitivity = 1;
        //! Integrate a held axis rate into displacement; mouse delta is already displacement.
        bool m_integrate = false;

        static Binding WASD();
        static Binding ArrowKeys();

        [[nodiscard]] bool Validate() const;
        [[nodiscard]] bool Supports(ActionType type) const;
        [[nodiscard]] bool IsDisplacement() const;
    };


    enum class MappingOperation : uint32_t
    {
        kSum,
        kStrongest,
    };


    struct ActionMapping final
    {
        FE_RTTI_Reflect("16158613-2631-4F6F-A38E-43FBFADE7D40");
        FE_RTTI_Serialize();

        IO::Link<Action> m_action;
        festd::vector<Binding> m_bindings;
        MappingOperation m_operation = MappingOperation::kSum;
        bool m_clampMagnitude = false;
        bool m_consume = false;
    };


    struct MappingTable final
    {
        FE_RTTI_Reflect("C27E593F-6199-41FB-AFFD-8E73F51ECCFE");
        FE_RTTI_Serialize();

        festd::vector<ActionMapping> m_mappings;

        [[nodiscard]] bool Validate() const;
    };


    struct ContextDefinition final
    {
        FE_RTTI_Reflect("F30B08EC-0FC4-40FD-A8FA-2947319CBAF8");
        FE_RTTI_Serialize();

        IO::Link<MappingTable> m_table;
        int32_t m_priority = 0;
        bool m_modal = false;
    };
} // namespace FE::Input
