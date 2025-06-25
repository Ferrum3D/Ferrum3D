#include <EASTL/bitset.h>
#include <FeCore/Memory/FiberTempAllocator.h>
#include <FeCore/Memory/PoolAllocator.h>
#include <Framework/Entities/Archetype.h>
#include <Framework/Entities/Entity.h>
#include <Framework/Entities/EntityComponentRegistry.h>
#include <Framework/Entities/EntityRegistry.h>
#include <Framework/Entities/EntitySystem.h>
#include <Framework/Entities/EntityWorld.h>

namespace FE::Framework
{
    namespace
    {
        // Systems should not be able to access the entity, so we pass this proxy provider instead
        struct TempComponentProvider final : public IComponentProvider
        {
            [[nodiscard]] void* GetComponentByTypeID(const ComponentTypeID componentType) const override
            {
                return m_entity->GetComponentByTypeID(componentType);
            }

            [[nodiscard]] void* SafeGetComponentByTypeID(const ComponentTypeID componentType) const override
            {
                return m_entity->SafeGetComponentByTypeID(componentType);
            }

            [[nodiscard]] bool HasComponentByTypeID(const ComponentTypeID componentType) const override
            {
                return m_entity->HasComponentByTypeID(componentType);
            }

            Entity* m_entity = nullptr;
        };


        Memory::SpinLockedPoolAllocator GEntityAllocator{ "EntityAllocator", sizeof(Entity) };
    } // namespace


    EntityID Entity::GetID() const
    {
        EntityID id;
        id.m_worldID = m_registry->GetWorld()->GetID();
        id.m_registryID = m_registry->GetID();
        id.m_entityID = m_entityIndexInRegistry;
        return id;
    }


    void Entity::AddSystem(EntitySystem* system)
    {
        const State state = m_state.load(std::memory_order_acquire);
        if (state == State::kUnloaded)
        {
            AddSystemImmediate(system);
        }
        else
        {
            AddDeferredAction(EntityDeferredAction::AddSystem(this, system));
            ++m_systemsToAddCount;
        }
    }


    void Entity::AddSystemImmediate(EntitySystem* system)
    {
        m_systems.push_back(system);

        const State state = m_state.load(std::memory_order_acquire);
        if (state == State::kInitialized)
        {
            system->Init();

            TempComponentProvider provider;
            provider.m_entity = this;

            if (m_archetypeChunk != nullptr)
                system->OnArchetypeChanged(&provider, m_archetypeChunk->m_archetype->m_componentTypeIDs, {});
            else
                system->OnArchetypeChanged(&provider, {}, {});
        }
    }


    void Entity::RemoveSystem(EntitySystem* system)
    {
        const State state = m_state.load(std::memory_order_acquire);
        if (state == State::kUnloaded)
            RemoveSystemImmediate(system);
        else
            AddDeferredAction(EntityDeferredAction::RemoveSystem(this, system));
    }


    void Entity::RemoveSystemImmediate(EntitySystem* system)
    {
        const auto it = festd::find(m_systems, system);
        FE_Assert(it != m_systems.end());

        m_systems.erase(it);
        system->Shutdown();
        system->Destroy();
    }


    void Entity::ChangeArchetypeImmediate(const festd::span<const ComponentTypeID> addedComponents,
                                          const festd::span<const ComponentTypeID> removedComponents)
    {
        std::lock_guard lock{ m_lock };

        Memory::FiberTempAllocator temp;

        const uint32_t componentCount = m_archetypeChunk ? m_archetypeChunk->m_archetype->m_componentTypes.size() : 0;

        festd::pmr::vector<ComponentTypeID> componentTypes{ &temp };
        componentTypes.reserve(componentCount + addedComponents.size() - removedComponents.size());
        componentTypes.resize(addedComponents.size());
        memcpy(componentTypes.data(), addedComponents.data(), festd::size_bytes(addedComponents));

        eastl::bitset<kMaxComponentsPerEntity> removedComponentsSet;
        if (m_archetypeChunk != nullptr)
        {
            const Archetype* archetype = m_archetypeChunk->m_archetype;
            for (uint32_t componentIndex = 0; componentIndex < componentCount; ++componentIndex)
            {
                const ComponentTypeID typeID = archetype->m_componentTypeIDs[componentIndex];
                const EntityComponentInfo* info = archetype->m_componentTypes[componentIndex];
                if (festd::find(removedComponents, typeID) != removedComponents.end())
                {
                    void* component = m_archetypeChunk->GetComponentData(m_entityIndexInArchetypeChunk, componentIndex);

                    if (info->m_shutdown != nullptr)
                        info->m_shutdown(component);

                    info->m_destroy(component);
                    removedComponentsSet.set(componentIndex);
                }
                else
                {
                    componentTypes.push_back(typeID);
                }
            }
        }

        Archetype* newArchetype = m_registry->GetArchetype(componentTypes);
        const auto [newChunk, newIndex] = newArchetype ? newArchetype->AllocateEntity() : EntityAllocationResult::kInvalid;

        eastl::bitset<kMaxComponentsPerEntity> constructedComponents;
        if (m_archetypeChunk != nullptr)
        {
            uint32_t newComponentIndex = 0;
            for (uint32_t componentIndex = 0; componentIndex < componentCount; ++componentIndex)
            {
                if (!removedComponentsSet.test(componentIndex))
                {
                    const ComponentTypeID typeID = newChunk->m_archetype->m_componentTypeIDs[componentIndex];
                    const EntityComponentInfo* info = newChunk->m_archetype->m_componentTypes[componentIndex];

                    for (; newComponentIndex < newArchetype->m_componentTypes.size(); ++newComponentIndex)
                    {
                        if (newArchetype->m_componentTypeIDs[newComponentIndex] == typeID)
                            break;
                    }

                    FE_AssertDebug(newComponentIndex < newArchetype->m_componentTypes.size());

                    void* src = m_archetypeChunk->GetComponentData(m_entityIndexInArchetypeChunk, componentIndex);
                    void* dst = newChunk->GetComponentData(newIndex, newComponentIndex);
                    info->m_moveConstruct(dst, src);
                    constructedComponents.set(newComponentIndex);
                }
            }

            m_archetypeChunk->Free(m_entityIndexInArchetypeChunk);
            m_archetypeChunk = nullptr;
        }

        if (newArchetype != nullptr)
        {
            for (uint32_t componentIndex = 0; componentIndex < newArchetype->m_componentTypes.size(); ++componentIndex)
            {
                if (!constructedComponents.test(componentIndex))
                {
                    const EntityComponentInfo* info = newChunk->m_archetype->m_componentTypes[componentIndex];
                    void* component = newChunk->GetComponentData(newIndex, componentIndex);
                    info->m_construct(component);

                    if (info->m_init != nullptr)
                        info->m_init(component);
                }
            }
        }

        m_archetypeChunk = newChunk;
        m_entityIndexInArchetypeChunk = newIndex;
    }


    Entity* Entity::Create(const Env::Name name, EntityRegistry* registry)
    {
        return new (GEntityAllocator.allocate(sizeof(Entity), alignof(Entity))) Entity(name, registry);
    }


    void Entity::Destroy(const Entity* entity)
    {
        entity->~Entity();
        GEntityAllocator.deallocate(const_cast<Entity*>(entity), sizeof(Entity), alignof(Entity));
    }


    void* Entity::GetComponentByTypeID(const ComponentTypeID componentType) const
    {
        FE_Assert(m_archetypeChunk);

        const Archetype* archetype = m_archetypeChunk->m_archetype;
        const uint32_t componentIndex = festd::find_index(archetype->m_componentTypeIDs, componentType);
        return m_archetypeChunk->GetComponentData(m_entityIndexInArchetypeChunk, componentIndex);
    }


    void* Entity::SafeGetComponentByTypeID(ComponentTypeID componentType) const
    {
        if (m_archetypeChunk == nullptr)
            return nullptr;

        const Archetype* archetype = m_archetypeChunk->m_archetype;
        const uint32_t componentIndex = festd::find_index(archetype->m_componentTypeIDs, componentType);
        if (componentIndex == kInvalidIndex)
            return nullptr;

        return m_archetypeChunk->GetComponentData(m_entityIndexInArchetypeChunk, componentIndex);
    }


    void Entity::AddComponentByTypeID(const ComponentTypeID componentType)
    {
        FE_Assert(m_state != State::kUnloaded);

        AddDeferredAction(EntityDeferredAction::AddComponent(this, componentType));
        ++m_componentsToAddCount;
    }


    bool Entity::HasComponentByTypeID(const ComponentTypeID componentType) const
    {
        if (m_archetypeChunk == nullptr)
            return false;

        const Archetype* archetype = m_archetypeChunk->m_archetype;
        const uint32_t componentIndex = festd::find_index(archetype->m_componentTypeIDs, componentType);
        return componentIndex != kInvalidIndex;
    }


    void Entity::ExecuteDeferredActions()
    {
        std::unique_lock lock{ m_lock };

        Memory::FiberTempAllocator temp;
        festd::pmr::vector<ComponentTypeID> addedComponents{ &temp };
        festd::pmr::vector<ComponentTypeID> removedComponents{ &temp };
        festd::pmr::vector<EntitySystem*> addedSystems{ &temp };
        festd::pmr::vector<EntitySystem*> removedSystems{ &temp };
        addedComponents.reserve(m_componentsToAddCount);
        removedComponents.reserve(m_componentsToRemoveCount);
        addedSystems.reserve(m_systemsToAddCount);
        removedSystems.reserve(m_systemsToRemoveCount);

        EntityDeferredAction* action = m_deferredActions;
        while (action != nullptr)
        {
            switch (action->m_type)
            {
            case EntityDeferredAction::Type::kAddSystem:
                addedSystems.push_back(action->m_data.m_system);
                break;

            case EntityDeferredAction::Type::kRemoveSystem:
                removedSystems.push_back(action->m_data.m_system);
                break;

            case EntityDeferredAction::Type::kAddComponent:
                addedComponents.push_back(action->m_data.m_componentType);
                break;

            case EntityDeferredAction::Type::kRemoveComponent:
                removedComponents.push_back(action->m_data.m_componentType);
                break;

            case EntityDeferredAction::Type::kInvalid:
            default:
                FE_DebugBreak();
                break;
            }

            action = action->m_next;
        }
        m_deferredActions = nullptr;

        for (EntitySystem* system : removedSystems)
            RemoveSystemImmediate(system);

        if (!addedComponents.empty() || !removedComponents.empty())
        {
            lock.unlock();
            ChangeArchetypeImmediate(addedComponents, removedComponents);

            lock.lock();

            TempComponentProvider provider;
            provider.m_entity = this;

            for (EntitySystem* system : m_systems)
                system->OnArchetypeChanged(&provider, addedComponents, removedComponents);
        }

        for (EntitySystem* system : addedSystems)
            AddSystemImmediate(system);
    }


    void Entity::UpdateLocalSystems(const EntityUpdateContext& context)
    {
        std::lock_guard lock{ m_lock };

        FE_Assert(m_state == State::kInitialized);
        for (EntitySystem* system : m_systems)
            system->Update(context);
    }


    Entity::Entity(const Env::Name name, EntityRegistry* registry)
        : m_name(name)
        , m_registry(registry)
    {
    }


    void Entity::AddDeferredAction(const EntityDeferredAction& action)
    {
        auto& allocator = m_registry->m_deferredActionsAllocator;
        auto* deferredAction = Memory::New<EntityDeferredAction>(&allocator, action);

        std::lock_guard lock{ m_lock };
        deferredAction->m_next = m_deferredActions;
        m_deferredActions = deferredAction;
    }


    void Entity::LoadComponents(const EntityLoadingContext& context)
    {
        FE_Assert(m_state.load(std::memory_order_acquire) == State::kUnloaded);

        std::lock_guard lock{ m_lock };

        if (m_archetypeChunk != nullptr)
        {
            const Archetype* archetype = m_archetypeChunk->m_archetype;
            for (uint32_t componentIndex = 0; componentIndex < archetype->m_componentTypes.size(); ++componentIndex)
            {
                const EntityComponentInfo* info = archetype->m_componentTypes[componentIndex];
                void* component = m_archetypeChunk->GetComponentData(m_entityIndexInArchetypeChunk, componentIndex);

                if (info->m_load != nullptr)
                    info->m_load(component, context);
            }
        }

        m_state.store(State::kLoaded, std::memory_order_release);
    }


    void Entity::UnloadComponents(const EntityLoadingContext& context)
    {
        FE_Assert(m_state.load(std::memory_order_acquire) == State::kLoaded);

        std::lock_guard lock{ m_lock };

        if (m_archetypeChunk != nullptr)
        {
            const Archetype* archetype = m_archetypeChunk->m_archetype;
            for (uint32_t componentIndex = 0; componentIndex < archetype->m_componentTypes.size(); ++componentIndex)
            {
                const EntityComponentInfo* info = archetype->m_componentTypes[componentIndex];
                void* component = m_archetypeChunk->GetComponentData(m_entityIndexInArchetypeChunk, componentIndex);

                if (info->m_unload != nullptr)
                    info->m_unload(component, context);
            }
        }

        m_state.store(State::kUnloaded, std::memory_order_release);
    }
} // namespace FE::Framework
