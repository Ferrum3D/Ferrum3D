#include <Core/Threading/Thread.h>
#include <Framework/Entities/EntityCommandListInternal.h>
#include <Framework/Entities/EntityWorldInternal.h>
#include <festd/bit_vector.h>

namespace FE::Framework
{
    using Internal::Command;
    using Internal::CommandKind;

    namespace
    {
        constexpr uint32_t kCommandListIDCount = 1u << 16;

        struct CommandListIDPool
        {
            CommandListIDPool()
            {
                m_available.resize(kCommandListIDCount, true);
                m_available.reset(0);
                m_shared.resize(kCommandListIDCount, false);
            }


            uint16_t Allocate()
            {
                std::lock_guard lock{ m_lock };
                const uint32_t id = m_available.find_first();
                FE_Assert(id != kInvalidIndex, "Too many live entity command lists");
                m_available.reset(id);
                return static_cast<uint16_t>(id);
            }


            uint16_t Share(const uint16_t id)
            {
                std::lock_guard lock{ m_lock };
                FE_AssertDebug(id != 0 && !m_available.test(id));
                FE_Assert(!m_shared.test(id), "A command list can split only once");
                m_shared.set(id);
                return id;
            }


            void Release(const uint16_t id)
            {
                std::lock_guard lock{ m_lock };
                FE_AssertDebug(id != 0 && !m_available.test(id));
                if (m_shared.test(id))
                {
                    m_shared.reset(id);
                    return;
                }

                m_available.set(id);
            }

        private:
            Threading::SpinLock m_lock;
            festd::fixed_bit_vector<kCommandListIDCount> m_available;
            // Submission creates at most two fragments; one extra bit retains their shared identity.
            festd::fixed_bit_vector<kCommandListIDCount> m_shared;
        };


        CommandListIDPool& GetCommandListIDPool()
        {
            static CommandListIDPool pool;
            return pool;
        }
    } // namespace


    uint16_t Internal::AllocateCommandListID()
    {
        return GetCommandListIDPool().Allocate();
    }


    uint16_t Internal::ShareCommandListID(const uint16_t id)
    {
        return GetCommandListIDPool().Share(id);
    }


    void Internal::ReleaseCommandListID(const uint16_t id)
    {
        GetCommandListIDPool().Release(id);
    }


    void EntityCommandList::Impl::Record(const Command& command)
    {
        const auto validateToken = [&]([[maybe_unused]] const EntityToken token) {
            FE_AssertDebug(!token.m_list || token.m_list == m_id, "Creation tokens cannot cross command lists");
            FE_AssertDebug(!token.m_list || token.m_index < m_created, "Creation token was not recorded by this list");
        };
        validateToken(command.GetTarget().m_token);
        validateToken(command.GetParent().m_token);
        if (const uint32_t registryID = command.GetRegistryID())
            m_world->m_impl->m_storage.m_registryIds.Retain(registryID);

        m_commands.push_back(command);
    }


    EntityCommandList::Impl::~Impl()
    {
        for (const auto& command : m_commands)
        {
            if (command.m_kind == CommandKind::kComponent && command.m_payload.m_component.m_value)
            {
                const auto& component = command.m_payload.m_component;
                component.m_info->m_type->m_destructor(component.m_value);
            }

            if (const uint32_t registryID = command.GetRegistryID())
                m_world->m_impl->m_storage.m_registryIds.Release(registryID);
        }

        for (const auto& value : m_ownedValues)
            value.m_info->m_type->m_destructor(value.m_data);

        for (void* payload : m_largePayloads)
            Memory::DefaultFree(payload);

        Internal::ReleaseCommandListID(m_id);
    }


    EntityCommandList::EntityCommandList(EntityWorld& world)
        : m_impl(Memory::DefaultNew<Impl>(world))
    {
    }


    EntityCommandList::~EntityCommandList()
    {
        Memory::DefaultDelete(m_impl);
    }


    EntityCommandList::EntityCommandList(EntityCommandList&& other) noexcept
        : m_impl(std::exchange(other.m_impl, nullptr))
    {
    }


    EntityCommandList& EntityCommandList::operator=(EntityCommandList&& other) noexcept
    {
        if (this != &other)
        {
            Memory::DefaultDelete(m_impl);
            m_impl = std::exchange(other.m_impl, nullptr);
        }

        return *this;
    }


    EntityComponentRegistry& EntityCommandList::Components()
    {
        return m_impl->m_world->Components();
    }


    void* EntityCommandList::AllocatePayload(const size_t size, const size_t alignment)
    {
        void* result = m_impl->m_arena.allocate(size, alignment);
        if (!result)
        {
            result = Memory::DefaultAllocate(size, alignment);
            m_impl->m_largePayloads.push_back(result);
        }

        return result;
    }


    EntityToken EntityCommandList::CreateEntity(EntityRegistry& registry, const Env::Name name, const Uuid uuid,
                                                const ResidencyScope residency)
    {
        FE_AssertDebug(m_impl && &registry.GetWorld() == m_impl->m_world);
        FE_Assert(m_impl->m_created < UINT16_MAX, "Too many entity creations in one command list");

        EntityToken token{ m_impl->m_id, m_impl->m_created++ };
        Command command{ CommandKind::kCreate, token };
        auto& creation = command.m_payload.m_create;
        creation.m_registry = &registry;
        creation.m_registryId = registry.GetID();
        creation.m_residency = residency;
        creation.m_name = name;
        creation.m_uuid = uuid.IsValid() ? uuid : Uuid::Random();
        m_impl->Record(command);
        return token;
    }


    void EntityCommandList::Destroy(const EntityTarget target)
    {
        m_impl->Record({ CommandKind::kDestroy, target });
    }


    void EntityCommandList::Rename(const EntityTarget target, const Env::Name name)
    {
        Command command{ CommandKind::kRename, target };
        command.m_payload.m_name = name;
        m_impl->Record(command);
    }


    void EntityCommandList::SetParent(const EntityTarget target, const EntityTarget parent, const ReparentMode mode)
    {
        Command command{ CommandKind::kParent, target };
        command.m_payload.m_parent = { parent, mode };
        m_impl->Record(command);
    }


    void EntityCommandList::SetActive(const EntityTarget target, const bool active)
    {
        Command command{ CommandKind::kActive, target };
        command.m_payload.m_active = active;
        m_impl->Record(command);
    }


    void EntityCommandList::Unload(const EntityTarget target)
    {
        m_impl->Record({ CommandKind::kUnload, target });
    }


    void EntityCommandList::UnloadRegistry(EntityRegistry& registry)
    {
        FE_AssertDebug(m_impl && &registry.GetWorld() == m_impl->m_world);

        Command command{ CommandKind::kUnloadRegistry, {} };
        command.m_payload.m_registry = { &registry, registry.GetID() };
        m_impl->Record(command);
    }


    void EntityCommandList::RemoveComponent(const EntityTarget target, const Rtti::TypeID type)
    {
        Command command{ CommandKind::kRemove, target };
        command.m_payload.m_remove = type;
        m_impl->Record(command);
    }


    bool EntityCommandList::ReplaceComponent(const EntityTarget target, const Rtti::Type& type, const void* value)
    {
        const auto* info = Components().Find(type.m_id);
        if (!info || !type.m_copyConstructor || !value)
            return false;

        void* storage = AllocatePayload(type.m_size, type.m_alignment);
        type.m_copyConstructor(storage, value);
        RecordComponent(target, *info, storage);
        return true;
    }


    void EntityCommandList::RecordComponent(const EntityTarget target, const EntityComponentInfo& info, void* storage)
    {
        Command command{ CommandKind::kComponent, target };
        command.m_payload.m_component = { &info, storage };
        m_impl->Record(command);
    }
} // namespace FE::Framework
