#include <Framework/Input/MappingTable.h>
#include <cmath>

namespace FE::Input
{
    bool Control::Validate() const
    {
        if (m_kind == ControlKind::kKey)
            return m_key != Core::Key::kInvalid && static_cast<uint32_t>(m_key) < 256;

        return m_kind == ControlKind::kMouseButton && m_button < 5;
    }


    Binding Binding::WASD()
    {
        Binding result;
        result.m_kind = BindingKind::kAxis2D;
        result.m_controls = { Control::Key(Core::Key::kA),
                              Control::Key(Core::Key::kD),
                              Control::Key(Core::Key::kS),
                              Control::Key(Core::Key::kW) };
        return result;
    }


    Binding Binding::ArrowKeys()
    {
        Binding result;
        result.m_kind = BindingKind::kAxis2D;
        result.m_controls = { Control::Key(Core::Key::kLeftArrow),
                              Control::Key(Core::Key::kRightArrow),
                              Control::Key(Core::Key::kDownArrow),
                              Control::Key(Core::Key::kUpArrow) };
        return result;
    }


    bool Binding::Validate(const ActionType type) const
    {
        if (!std::isfinite(m_sensitivity) || !std::isfinite(m_scale.x) || !std::isfinite(m_scale.y))
            return false;

        for (const Control& control : m_controls)
        {
            if (!control.Validate())
                return false;
        }

        for (const Control& condition : m_conditions)
        {
            if (!condition.Validate())
                return false;
        }

        switch (m_kind)
        {
        case BindingKind::kButton:
            return m_controls.size() == 1;
        case BindingKind::kAxis1D:
            return m_controls.size() == 2 && type == ActionType::kScalar;
        case BindingKind::kAxis2D:
            return m_controls.size() == 4 && type == ActionType::kVector2;
        case BindingKind::kMouseDelta:
            return m_controls.empty() && type == ActionType::kVector2 && !m_integrate;
        case BindingKind::kMouseWheel:
            return m_controls.empty() && type != ActionType::kBoolean && !m_integrate;
        case BindingKind::kChord:
            return !m_controls.empty();
        default:
            return false;
        }
    }


    bool MappingTable::Validate() const
    {
        for (uint32_t i = 0; i < m_mappings.size(); ++i)
        {
            const ActionMapping& mapping = m_mappings[i];
            if (mapping.m_action.GetAssetID() == IO::AssetID::kNull || mapping.m_type > ActionType::kVector2
                || mapping.m_operation > MappingOperation::kStrongest || mapping.m_bindings.empty())
            {
                return false;
            }

            for (uint32_t j = 0; j < i; ++j)
            {
                if (m_mappings[j].m_action.GetAssetID() == mapping.m_action.GetAssetID())
                    return false;
            }

            const auto isDisplacement = [](const Binding& binding) {
                return binding.m_integrate || binding.m_kind == BindingKind::kMouseDelta
                    || binding.m_kind == BindingKind::kMouseWheel;
            };
            const bool displacement = isDisplacement(mapping.m_bindings.front());
            for (const Binding& binding : mapping.m_bindings)
            {
                if (!binding.Validate(mapping.m_type) || isDisplacement(binding) != displacement)
                    return false;
            }
        }

        return true;
    }
} // namespace FE::Input
