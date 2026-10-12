#pragma once
#include <Framework/Input/Action.h>
#include <Framework/Input/Core/Keys.h>

namespace FE::Input
{
    enum class ControlKind : uint32_t
    {
        kKey,
        kMouseButton,
    };


    struct Control final
    {
        FE_RTTI_Reflect("26C1E4CC-F60E-4474-83EB-C892B0301E4D");
        FE_RTTI_Serialize();

        ControlKind m_kind = ControlKind::kKey;
        Core::Key m_key = Core::Key::kInvalid;
        uint32_t m_button = 0;

        static Control Key(const Core::Key key)
        {
            return { ControlKind::kKey, key, 0 };
        }

        static Control MouseButton(const uint32_t button)
        {
            return { ControlKind::kMouseButton, Core::Key::kInvalid, button };
        }

        [[nodiscard]] bool Validate() const;
    };


    enum class BindingKind : uint32_t
    {
        kButton,
        kAxis1D,
        kAxis2D,
        kMouseDelta,
        kMouseWheel,
        kChord,
    };


    //! Axis1D controls are negative/positive; Axis2D controls are left/right/back/forward.
    struct Binding final
    {
        FE_RTTI_Reflect("AF710BC7-D755-4116-A5DC-F44202849DFC");
        FE_RTTI_Serialize();

        BindingKind m_kind = BindingKind::kButton;
        festd::vector<Control> m_controls;
        festd::vector<Control> m_conditions;
        Vector2 m_scale{ 1, 1 };
        float m_sensitivity = 1;
        //! Integrate a held axis rate into displacement; mouse delta is already displacement.
        bool m_integrate = false;
        static Binding WASD();
        static Binding ArrowKeys();
        [[nodiscard]] bool Validate(ActionType type) const;
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
        //! Explicit expected type permits validation before asset dependencies are loaded.
        ActionType m_type = ActionType::kBoolean;
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
