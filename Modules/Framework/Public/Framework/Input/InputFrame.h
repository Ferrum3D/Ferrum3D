#pragma once
#include <Framework/Input/Action.h>
#include <concepts>
#include <festd/unordered_map.h>

namespace FE::Input
{
    template<class T>
    concept ActionValue = std::same_as<T, Vector2> || std::same_as<T, float> || std::same_as<T, bool>;


    template<ActionValue T>
    inline constexpr ActionType kActionType = std::same_as<T, Vector2> ? ActionType::kVector2
        : std::same_as<T, float>                                       ? ActionType::kScalar
                                                                       : ActionType::kBoolean;


    template<ActionValue T>
    struct ActionReference final
    {
        ActionReference() = default;

        ActionReference(const IO::Link<Action>& link)
            : m_link(link)
        {
        }

        [[nodiscard]] const IO::Link<Action>& GetLink() const
        {
            return m_link;
        }

        [[nodiscard]] bool Validate() const
        {
            if (!m_link.GetAssetID().IsValid())
                return true;

            const auto action = m_link.GetAssetHandle().Read();
            return !action || action->m_type == kActionType<T>;
        }

    private:
        IO::Link<Action> m_link;
    };


    //! Stable values and ordered events until the world's next input update.
    struct InputFrame final
    {
        template<ActionValue T>
        [[nodiscard]] T Read(const ActionReference<T>& reference) const
        {
            const auto found = m_indices.find(reference.GetLink().GetAssetID());
            if (found == m_indices.end())
                return T{};

            const ResolvedAction& action = m_actions[found->second];
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
        friend struct MappingStack;

        festd::unordered_dense_map<IO::AssetID, uint32_t> m_indices;
        uint64_t m_number = 0;
        festd::vector<ResolvedAction> m_actions;
        festd::vector<ActionEvent> m_events;
    };
} // namespace FE::Input


namespace FE::Serialization
{
    template<Input::ActionValue T>
    struct Serializer<Input::ActionReference<T>>
    {
        static ResultCode Serialize(SerializationContext& context, const Input::ActionReference<T>& reference)
        {
            return SerializeValue(context, reference.GetLink());
        }

        static ResultCode Deserialize(DeserializationContext& context, Input::ActionReference<T>& reference)
        {
            IO::Link<Input::Action> link;
            const ResultCode result = DeserializeValue(context, link);
            if (result == ResultCode::kSuccess)
                reference = link;
            return result;
        }

        static uint64_t GetSchemaHash()
        {
            return Serialization::GetSchemaHash<IO::Link<Input::Action>>();
        }
    };
} // namespace FE::Serialization


namespace FE::Rtti
{
    //! An action reference is an asset reference to its action, like the link it wraps.
    template<Input::ActionValue T>
    struct Shape<Input::ActionReference<T>>
    {
        using Reference = Input::ActionReference<T>;

        static void Bind(Type& type)
        {
            static constexpr HandleShape kShape{
                .m_get =
                    [](const void* value) {
                        return static_cast<const Reference*>(value)->GetLink().GetAssetID();
                    },
                .m_set =
                    [](void* value, const Uuid uuid) {
                        *static_cast<Reference*>(value) = IO::Link<Input::Action>(uuid);
                    },
            };
            IO::Internal::AssetReferenceShape<Input::Action, IO::DependencyKind::kHard>::Bind(type, kShape);
        }
    };
} // namespace FE::Rtti
