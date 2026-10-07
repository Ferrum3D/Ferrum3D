#include <Core/Threading/Thread.h>
#include <Framework/Entities/EntityRuntime.h>
#include <Windows.h>
#include <bcrypt.h>

#pragma comment(lib, "bcrypt.lib")

namespace FE::Framework
{
    Uuid NewEntityUuid()
    {
        Uuid result{ kForceInit };
        FE_Verify(BCryptGenRandom(nullptr, result.data(), 16, BCRYPT_USE_SYSTEM_PREFERRED_RNG) >= 0);
        result.m_bytes[6] = (result.m_bytes[6] & 0xf) | 0x40;
        result.m_bytes[8] = (result.m_bytes[8] & 0x3f) | 0x80;
        return result;
    }


    uint64_t Internal::NextCommandListID()
    {
        static std::atomic<uint64_t> nextList{ 1 };
        return nextList.fetch_add(1);
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
        EntityToken token{ m_impl->m_id, m_impl->m_created++ };
        Command command{ CommandKind::kCreate, token };
        command.m_registry = &registry;
        command.m_registryId = registry.GetID();
        command.m_residency = residency;
        command.m_name = name;
        command.m_uuid = uuid.IsValid() ? uuid : NewEntityUuid();
        m_impl->m_commands.push_back(command);
        return token;
    }


    void EntityCommandList::Destroy(const EntityTarget target)
    {
        m_impl->m_commands.push_back({ CommandKind::kDestroy, target });
    }


    void EntityCommandList::Rename(const EntityTarget target, const Env::Name name)
    {
        Command command{ CommandKind::kRename, target };
        command.m_name = name;
        m_impl->m_commands.push_back(command);
    }


    void EntityCommandList::SetParent(const EntityTarget target, const EntityTarget parent, const ReparentMode mode)
    {
        Command command{ CommandKind::kParent, target };
        command.m_parent = parent;
        command.m_reparentMode = mode;
        m_impl->m_commands.push_back(command);
    }


    void EntityCommandList::SetActive(const EntityTarget target, const bool active)
    {
        Command command{ CommandKind::kActive, target };
        command.m_active = active;
        m_impl->m_commands.push_back(command);
    }


    void EntityCommandList::Unload(const EntityTarget target)
    {
        m_impl->m_commands.push_back({ CommandKind::kUnload, target });
    }


    void EntityCommandList::UnloadRegistry(EntityRegistry& registry)
    {
        Command command{ CommandKind::kUnloadRegistry, {} };
        command.m_registry = &registry;
        command.m_registryId = registry.GetID();
        m_impl->m_commands.push_back(command);
    }


    void EntityCommandList::RemoveComponent(const EntityTarget target, const Rtti::TypeID type)
    {
        Command command{ CommandKind::kRemove, target };
        command.m_type = type;
        m_impl->m_commands.push_back(command);
    }


    void EntityCommandList::RecordComponent(const EntityTarget target, const EntityComponentInfo& info, void* storage)
    {
        Command command{ CommandKind::kComponent, target };
        command.m_component = &info;
        command.m_type = info.m_type->m_id;
        command.m_payload = storage;
        m_impl->m_commands.push_back(command);
    }
} // namespace FE::Framework
