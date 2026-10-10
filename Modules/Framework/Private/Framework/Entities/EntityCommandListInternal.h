#pragma once
#include <Framework/Entities/EntityCommandList.h>
#include <memory>

namespace FE::Framework
{
    namespace Internal
    {
        uint16_t AllocateCommandListID();
        uint16_t ShareCommandListID(uint16_t id);
        void ReleaseCommandListID(uint16_t id);

        enum class CommandKind : uint8_t
        {
            kCreate,
            kDestroy,
            kRename,
            kParent,
            kActive,
            kUnload,
            kUnloadRegistry,
            kComponent,
            kRemove
        };


        struct Command
        {
            union Target
            {
                EntityID m_id;
                EntityToken m_token;
            };

            struct Creation
            {
                Uuid m_uuid = Uuid::kNull;
                EntityRegistry* m_registry = nullptr;
                uint32_t m_registryId = 0;
                Env::Name m_name;
                ResidencyScope m_residency = ResidencyScope::kEntity;
            };

            struct Parenting
            {
                EntityTarget m_target;
                ReparentMode m_mode = ReparentMode::kPreserveWorld;
            };

            struct Component
            {
                const EntityComponentInfo* m_info = nullptr;
                void* m_value = nullptr;
            };

            struct Registry
            {
                EntityRegistry* m_registry = nullptr;
                uint32_t m_id = 0;
            };


            union Payload
            {
                Creation m_create;
                Parenting m_parent;
                Component m_component;
                Registry m_registry;
                Rtti::TypeID m_remove;
                Env::Name m_name;
                bool m_active;

                Payload() {}
            };

            Target m_target;
            CommandKind m_kind;
            bool m_tokenTarget;
            Payload m_payload;

            Command(const CommandKind kind, const EntityTarget target)
                : m_target(target.m_token.m_list ? Target{ .m_token = target.m_token } : Target{ .m_id = target.m_id })
                , m_kind(kind)
                , m_tokenTarget(target.m_token.m_list != 0)
            {
                // Explicit construction selects the active union member, including types with nontrivial defaults.
                switch (kind)
                {
                case CommandKind::kCreate:
                    std::construct_at(&m_payload.m_create);
                    break;

                case CommandKind::kParent:
                    std::construct_at(&m_payload.m_parent);
                    break;

                case CommandKind::kComponent:
                    std::construct_at(&m_payload.m_component);
                    break;

                case CommandKind::kUnloadRegistry:
                    std::construct_at(&m_payload.m_registry);
                    break;

                case CommandKind::kRemove:
                    std::construct_at(&m_payload.m_remove, Rtti::TypeID::kNull);
                    break;

                case CommandKind::kRename:
                    std::construct_at(&m_payload.m_name);
                    break;

                case CommandKind::kActive:
                    std::construct_at(&m_payload.m_active, true);
                    break;

                case CommandKind::kDestroy:
                case CommandKind::kUnload:
                    break;
                }
            }


            EntityTarget GetTarget() const
            {
                return m_tokenTarget ? EntityTarget(m_target.m_token) : EntityTarget(m_target.m_id);
            }


            EntityTarget GetParent() const
            {
                return m_kind == CommandKind::kParent ? m_payload.m_parent.m_target : EntityTarget{};
            }


            EntityRegistry* GetRegistry() const
            {
                if (m_kind == CommandKind::kCreate)
                    return m_payload.m_create.m_registry;

                if (m_kind == CommandKind::kUnloadRegistry)
                    return m_payload.m_registry.m_registry;

                return nullptr;
            }


            uint32_t GetRegistryID() const
            {
                if (m_kind == CommandKind::kCreate)
                    return m_payload.m_create.m_registryId;

                if (m_kind == CommandKind::kUnloadRegistry)
                    return m_payload.m_registry.m_id;

                return 0;
            }
        };

        static_assert(sizeof(Command) == 64);
        static_assert(std::is_trivially_copyable_v<Command>);
    } // namespace Internal


    struct EntityCommandList::Impl
    {
        EntityWorld* m_world;
        uint64_t m_eligibleEpoch = 0;
        uint16_t m_id;
        uint16_t m_created = 0;

        Memory::LinearAllocator m_arena;
        festd::inline_vector<Internal::Command> m_commands;

        struct OwnedValue
        {
            const EntityComponentInfo* m_info;
            void* m_data;
        };

        festd::vector<OwnedValue> m_ownedValues;
        festd::vector<void*> m_largePayloads;

        explicit Impl(EntityWorld& world)
            : m_world(&world)
            , m_id(Internal::AllocateCommandListID())
        {
        }


        Impl(EntityWorld& world, const uint16_t sharedID)
            : m_world(&world)
            , m_id(Internal::ShareCommandListID(sharedID))
        {
        }

        void Record(const Internal::Command& command);
        ~Impl();
    };
} // namespace FE::Framework
