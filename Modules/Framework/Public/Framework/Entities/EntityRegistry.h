#pragma once
#include <Core/Containers/SegmentedVector.h>
#include <Core/Env/Environment.h>
#include <Core/Memory/LinearAllocator.h>
#include <Core/Memory/Memory.h>
#include <Framework/Entities/Base.h>
#include <festd/bit_vector.h>
#include <festd/unordered_map.h>

namespace FE::Framework
{
    struct EntityRegistry final : public Memory::RefCountedObjectBase
    {
        enum class State : uint32_t
        {
            kUnloaded,
            kLoading,
            kLoaded,
            kUnloading,
            kLoadingFailed,
        };

        FE_RTTI("D7D3E880-E152-4014-B598-A9A97F5E463C");

        explicit EntityRegistry(EntityWorld* world);
        ~EntityRegistry() override;

        [[nodiscard]] State GetState() const
        {
            return m_state;
        }

        [[nodiscard]] EntityWorld* GetWorld() const
        {
            return m_world;
        }

        [[nodiscard]] uint32_t GetID() const
        {
            return m_ID;
        }

        void RequestLoad();
        void RequestUnload();

        Entity* CreateEntity(Env::Name name);

        Entity* GetEntityByID(EntityID id) const;

        Archetype* GetArchetype(festd::span<const ComponentTypeID> componentTypes);

    private:
        friend Entity;
        friend EntityWorld;

        void DoRelease() override
        {
            Memory::DefaultDelete(this);
        }

        void UpdateLoadingState(const EntityLoadingContext& context);
        void LoadImpl(const EntityLoadingContext& context);
        void UnloadImpl(const EntityLoadingContext& context);

        void Update(const EntityUpdateContext& context);

        uint32_t m_ID = kInvalidEntityRegistryID;
        mutable Threading::SpinLock m_lock;
        std::atomic<State> m_state = State::kUnloaded;
        EntityWorld* m_world = nullptr;
        SegmentedVector<Archetype*> m_archetypes;
        uint32_t m_prevArchetypeCount = 0;

        festd::unordered_dense_map<uint64_t, Archetype*> m_archetypeMap;
        SegmentedVector<Entity*> m_entities;
        SegmentedVector<Entity*> m_entitiesUnsorted;
        festd::bit_vector m_entityFreeList;

        Memory::SpinLockedLinearAllocator m_deferredActionsAllocator;
    };
} // namespace FE::Framework
