#pragma once
#include <Framework/Input/Action.h>

namespace FE::Input
{
    template<class T>
    inline constexpr ActionType kActionType = std::same_as<T, Vector2> ? ActionType::kVector2
        : std::same_as<T, float>                                       ? ActionType::kScalar
                                                                       : ActionType::kBoolean;


    template<class T>
    struct ActionHandle final
    {
        static_assert(std::same_as<T, Vector2> || std::same_as<T, float> || std::same_as<T, bool>);
        uint32_t m_index = Constants::kMaxU32;
        uint64_t m_generation = 0;
    };


    //! Stable values and ordered events until the world's next input update.
    struct InputFrame final
    {
        template<class T>
        [[nodiscard]] T Read(const ActionHandle<T> handle) const
        {
            if (handle.m_generation != m_generation || handle.m_index >= m_actions.size())
                return T{};

            const ResolvedAction& action = m_actions[handle.m_index];
            if (!action.m_active || action.m_type != kActionType<T>)
                return T{};

            if constexpr (std::same_as<T, Vector2>)
                return action.m_value;
            else if constexpr (std::same_as<T, float>)
                return action.m_value.x;
            else
                return action.m_value.x != 0;
        }

        [[nodiscard]] festd::span<const ActionEvent> GetEvents() const
        {
            return m_events;
        }

        [[nodiscard]] uint64_t GetNumber() const
        {
            return m_number;
        }

    private:
        friend struct WorldInputService;
        uint64_t m_generation = 0;
        uint64_t m_number = 0;
        festd::vector<ResolvedAction> m_actions;
        festd::vector<ActionEvent> m_events;
    };
} // namespace FE::Input
