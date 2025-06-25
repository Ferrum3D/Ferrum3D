#pragma once
#include <FeCore/Modules/Environment.h>
#include <Framework/Entities/Base.h>
#include <Framework/Entities/EntityComponentRegistry.h>
#include <Framework/Entities/EntitySystem.h>
#include <festd/vector.h>

namespace FE::Framework
{
    struct EntityDeferredAction final
    {
        enum class Type : uint32_t
        {
            kInvalid,
            kAddSystem,
            kRemoveSystem,
            kAddComponent,
            kRemoveComponent,
        };

        EntityDeferredAction* m_next = nullptr;
        Type m_type = Type::kInvalid;
        Entity* m_entity = nullptr;

        union
        {
            EntitySystem* m_system = nullptr;
            ComponentTypeID m_componentType;
        } m_data;

        static EntityDeferredAction AddSystem(Entity* entity, EntitySystem* system)
        {
            EntityDeferredAction action;
            action.m_type = Type::kAddSystem;
            action.m_entity = entity;
            action.m_data.m_system = system;
            return action;
        }

        static EntityDeferredAction RemoveSystem(Entity* entity, EntitySystem* system)
        {
            EntityDeferredAction action;
            action.m_type = Type::kRemoveSystem;
            action.m_entity = entity;
            action.m_data.m_system = system;
            return action;
        }

        static EntityDeferredAction AddComponent(Entity* entity, const ComponentTypeID componentType)
        {
            EntityDeferredAction action;
            action.m_type = Type::kAddComponent;
            action.m_entity = entity;
            action.m_data.m_componentType = componentType;
            return action;
        }

        static EntityDeferredAction RemoveComponent(Entity* entity, const ComponentTypeID componentType)
        {
            EntityDeferredAction action;
            action.m_type = Type::kRemoveComponent;
            action.m_entity = entity;
            action.m_data.m_componentType = componentType;
            return action;
        }
    };


    union EntityID final
    {
        struct
        {
            uint32_t m_worldID : kEntityWorldIDBits;
            uint32_t m_registryID : kEntityRegistryIDBits;
            uint32_t m_entityID;
        };

        uint64_t m_value;

        EntityID() = default;

        explicit EntityID(ForceInitType)
        {
            m_value = Constants::kMaxU64;
        }

        static const EntityID kInvalid;
    };

    inline const EntityID EntityID::kInvalid{ kForceInit };

    static_assert(sizeof(EntityID) == sizeof(uint64_t));


    struct Entity final : public IComponentProvider
    {
        enum class State : uint32_t
        {
            kUnloaded,
            kLoaded,
            kInitialized,
        };

        [[nodiscard]] EntityID GetID() const;

        static Entity* Create(Env::Name name, EntityRegistry* registry);
        static void Destroy(const Entity* entity);

        [[nodiscard]] void* GetComponentByTypeID(ComponentTypeID componentType) const override;
        [[nodiscard]] void* SafeGetComponentByTypeID(ComponentTypeID componentType) const override;
        [[nodiscard]] bool HasComponentByTypeID(ComponentTypeID componentType) const override;

        template<class TComponent>
        void AddComponent()
        {
            constexpr ComponentTypeID componentType = ComponentTypeID::Create<TComponent>();
            EntityComponentRegistry::Get().RegisterComponent<TComponent>();
            AddComponentByTypeID(componentType);
        }

        void AddComponentByTypeID(ComponentTypeID componentType);

        void AddSystem(EntitySystem* system);
        void AddSystemImmediate(EntitySystem* system);

        void RemoveSystem(EntitySystem* system);
        void RemoveSystemImmediate(EntitySystem* system);

        void ChangeArchetypeImmediate(festd::span<const ComponentTypeID> addedComponents,
                                      festd::span<const ComponentTypeID> removedComponents);

        void ExecuteDeferredActions();

        void UpdateLocalSystems(const EntityUpdateContext& context);

    private:
        friend EntityRegistry;

        Entity(Env::Name name, EntityRegistry* registry);

        void AddDeferredAction(const EntityDeferredAction& action);
        void LoadComponents(const EntityLoadingContext& context);
        void UnloadComponents(const EntityLoadingContext& context);

        Threading::SpinLock m_lock;
        std::atomic<State> m_state = State::kUnloaded;
        uint32_t m_entityIndexInRegistry = kInvalidIndex;
        uint32_t m_entityIndexInArchetypeChunk = kInvalidIndex;
        Env::Name m_name;
        EntityRegistry* m_registry = nullptr;
        ArchetypeChunk* m_archetypeChunk = nullptr;
        festd::inline_vector<EntitySystem*> m_systems;

        EntityDeferredAction* m_deferredActions = nullptr;
        uint32_t m_componentsToAddCount = 0;
        uint32_t m_componentsToRemoveCount = 0;
        uint32_t m_systemsToAddCount = 0;
        uint32_t m_systemsToRemoveCount = 0;
    };
} // namespace FE::Framework
