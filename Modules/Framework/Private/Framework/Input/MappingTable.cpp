#include <Core/Logging/Logger.h>
#include <Framework/Input/MappingTable.h>
#include <algorithm>

namespace FE::Input
{
    bool Control::Validate() const
    {
        return m_code > 0 && m_code < 261;
    }


    Binding Binding::WASD()
    {
        Binding result;
        result.m_source = Axis2DBinding{ Control::Key(Core::Key::kA),
                                         Control::Key(Core::Key::kD),
                                         Control::Key(Core::Key::kS),
                                         Control::Key(Core::Key::kW) };
        return result;
    }


    Binding Binding::ArrowKeys()
    {
        Binding result;
        result.m_source = Axis2DBinding{ Control::Key(Core::Key::kLeftArrow),
                                         Control::Key(Core::Key::kRightArrow),
                                         Control::Key(Core::Key::kDownArrow),
                                         Control::Key(Core::Key::kUpArrow) };
        return result;
    }


    bool Binding::Validate() const
    {
        for (const Control& condition : m_conditions)
        {
            if (!condition.Validate())
                return false;
        }

        return std::visit(
            [&]<class T>(const T& source) {
                if constexpr (std::same_as<T, ButtonBinding>)
                    return source.m_control.Validate();
                else if constexpr (std::same_as<T, Axis1DBinding>)
                    return source.m_negative.Validate() && source.m_positive.Validate();
                else if constexpr (std::same_as<T, Axis2DBinding>)
                    return source.m_left.Validate() && source.m_right.Validate() && source.m_back.Validate()
                        && source.m_forward.Validate();
                else if constexpr (std::same_as<T, ChordBinding>)
                    return !source.m_controls.empty()
                        && std::all_of(source.m_controls.begin(), source.m_controls.end(), [](const Control& control) {
                               return control.Validate();
                           });
                else
                    return !m_integrate;
            },
            m_source);
    }


    bool Binding::Supports(const ActionType type) const
    {
        if (std::holds_alternative<Axis1DBinding>(m_source))
            return type == ActionType::kScalar;
        if (std::holds_alternative<Axis2DBinding>(m_source) || std::holds_alternative<MouseDeltaBinding>(m_source))
            return type == ActionType::kVector2;
        if (std::holds_alternative<MouseWheelBinding>(m_source))
            return type == ActionType::kScalar;

        return true;
    }


    bool Binding::IsDisplacement() const
    {
        return m_integrate || std::holds_alternative<MouseDeltaBinding>(m_source)
            || std::holds_alternative<MouseWheelBinding>(m_source);
    }


    bool MappingTable::Validate() const
    {
        for (uint32_t i = 0; i < m_mappings.size(); ++i)
        {
            const ActionMapping& mapping = m_mappings[i];
            if (mapping.m_action.GetAssetID() == IO::AssetID::kNull || mapping.m_operation > MappingOperation::kStrongest
                || mapping.m_bindings.empty())
            {
                Logger::LogError("Input mapping {} has an invalid action, operation or empty bindings", i);
                return false;
            }

            for (uint32_t j = 0; j < i; ++j)
            {
                if (m_mappings[j].m_action.GetAssetID() == mapping.m_action.GetAssetID())
                {
                    Logger::LogError("Input mapping {} duplicates action {}", i, mapping.m_action.GetAssetID());
                    return false;
                }
            }

            const bool displacement = mapping.m_bindings.front().IsDisplacement();
            for (uint32_t bindingIndex = 0; bindingIndex < mapping.m_bindings.size(); ++bindingIndex)
            {
                const Binding& binding = mapping.m_bindings[bindingIndex];
                const bool compatibleIntegration = binding.m_integrate == mapping.m_bindings.front().m_integrate;
                const bool mouseSource = std::holds_alternative<MouseDeltaBinding>(binding.m_source)
                    || std::holds_alternative<MouseWheelBinding>(binding.m_source);
                if (!binding.Validate() || binding.IsDisplacement() != displacement || !compatibleIntegration
                    || (mouseSource && mapping.m_clampMagnitude))
                {
                    Logger::LogError("Input mapping {} binding {} has invalid controls, finite parameters or incompatible units",
                                     i,
                                     bindingIndex);
                    return false;
                }
            }
        }

        return true;
    }
} // namespace FE::Input
