#pragma once
#include <Framework/Entities/EntityCommandList.h>

namespace FE::Framework
{
    namespace Internal
    {
        uint64_t NextCommandListID();

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
            Uuid m_uuid = Uuid::kNull;
            Rtti::TypeID m_type = Rtti::TypeID::kNull;
            EntityTarget m_target;
            EntityTarget m_parent;
            EntityRegistry* m_registry = nullptr;
            const EntityComponentInfo* m_component = nullptr;
            void* m_payload = nullptr;
            uint64_t m_registryId = 0;
            Env::Name m_name;
            CommandKind m_kind;
            bool m_active = true;
            ResidencyScope m_residency = ResidencyScope::kEntity;
            ReparentMode m_reparentMode = ReparentMode::kPreserveWorld;

            Command(CommandKind kind, EntityTarget target)
                : m_target(target)
                , m_kind(kind)
            {
            }
        };
    } // namespace Internal


    struct EntityCommandList::Impl
    {
        EntityWorld* m_world;
        uint64_t m_id = Internal::NextCommandListID();
        uint32_t m_created = 0;
        uint64_t m_eligibleEpoch = 0;
        Memory::LinearAllocator m_arena;
        festd::vector<Internal::Command> m_commands;
        struct OwnedValue
        {
            const EntityComponentInfo* m_info;
            void* m_data;
        };

        festd::vector<OwnedValue> m_ownedValues;
        festd::vector<void*> m_largePayloads;
        explicit Impl(EntityWorld& world)
            : m_world(&world)
        {
        }


        void Record(Internal::Command command)
        {
            const auto validateToken = [&](EntityToken token) {
                FE_Assert(!token.m_list || token.m_list == m_id, "Creation tokens cannot cross command lists");
                FE_Assert(!token.m_list || token.m_index < m_created, "Creation token was not recorded by this list");
            };
            validateToken(command.m_target.m_token);
            validateToken(command.m_parent.m_token);
            m_commands.push_back(command);
        }


        ~Impl()
        {
            for (const auto& command : m_commands)
            {
                if (command.m_payload)
                    command.m_component->m_type->m_destructor(command.m_payload);
            }

            for (const auto& value : m_ownedValues)
                value.m_info->m_type->m_destructor(value.m_data);

            for (void* payload : m_largePayloads)
                Memory::DefaultFree(payload);
        }
    };
} // namespace FE::Framework
