#include <FeCore/Jobs/Job.h>
#include <FeCore/Memory/FiberTempAllocator.h>
#include <Framework/Entities/Archetype.h>
#include <Framework/Entities/Entity.h>
#include <Framework/Entities/EntityRegistry.h>
#include <Framework/Entities/EntityWorld.h>

namespace FE::Framework
{
    namespace
    {
        struct DeferredActionJob final : public Job
        {
            void Execute() override
            {
                for (Entity* entity : m_entities)
                    entity->ExecuteDeferredActions();
            }

            festd::span<Entity*> m_entities;
        };


        struct LocalSystemUpdateJob final : public Job
        {
            void Execute() override
            {
                for (Entity* entity : m_entities)
                    entity->UpdateLocalSystems(*m_context);
            }

            festd::span<Entity*> m_entities;
            const EntityUpdateContext* m_context;
        };
    } // namespace

    void EntityRegistry::RequestLoad()
    {
        FE_Verify(m_state.exchange(State::kLoading) == State::kUnloaded);
    }


    void EntityRegistry::RequestUnload()
    {
        FE_Verify(m_state.exchange(State::kUnloading) != State::kUnloaded);
    }


    Entity* EntityRegistry::CreateEntity(const Env::Name name)
    {
        std::lock_guard lock{ m_lock };

        Entity* entity = Entity::Create(name, this);

        uint32_t id = m_entityFreeList.find_first();
        if (id == kInvalidIndex)
        {
            id = m_entities.size();
            m_entities.resize(m_entities.size() + 256);
            m_entityFreeList.resize(m_entityFreeList.size() + 256, true);
        }

        FE_Assert(m_entities[id] == nullptr);
        m_entities[id] = entity;
        m_entityFreeList.reset(id);
        m_entitiesUnsorted.push_back(entity);

        entity->m_entityIndexInRegistry = id;
        entity->m_state = Entity::State::kInitialized;

        return entity;
    }


    Entity* EntityRegistry::GetEntityByID(const EntityID id) const
    {
        std::lock_guard lock{ m_lock };

        FE_Assert(m_world->GetID() == id.m_worldID);
        FE_Assert(m_ID == id.m_registryID);
        return m_entities[id.m_entityID];
    }


    Archetype* EntityRegistry::GetArchetype(const festd::span<const ComponentTypeID> componentTypes)
    {
        std::lock_guard lock{ m_lock };

        if (componentTypes.empty())
            return nullptr;

        Memory::FiberTempAllocator temp;
        festd::pmr::vector<ComponentTypeID> types{ &temp };
        types.resize(componentTypes.size());
        memcpy(types.data(), componentTypes.data(), componentTypes.size() * sizeof(ComponentTypeID));
        festd::sort(types, [](const ComponentTypeID lhs, const ComponentTypeID rhs) {
            return lhs.m_value < rhs.m_value;
        });

        Hasher hasher;
        for (const ComponentTypeID type : types)
            hasher.UpdateRaw(type.m_value);

        const uint64_t hash = hasher.Finalize();
        const auto it = m_archetypeMap.find(hash);
        if (it != m_archetypeMap.end())
            return it->second;

        auto* archetype = Archetype::Create(this, types);
        m_archetypeMap[hash] = archetype;
        m_archetypes.push_back(archetype);
        return archetype;
    }


    EntityRegistry::EntityRegistry(EntityWorld* world)
        : m_world(world)
    {
    }


    EntityRegistry::~EntityRegistry()
    {
        for (const Entity* entity : m_entitiesUnsorted)
            Entity::Destroy(entity);

        for (const Archetype* archetype : m_archetypes)
            Archetype::Destroy(archetype);
    }


    void EntityRegistry::UpdateLoadingState(const EntityLoadingContext& context)
    {
        std::lock_guard lock{ m_lock };

        switch (m_state.load(std::memory_order_acquire))
        {
        case State::kLoading:
            LoadImpl(context);
            break;

        case State::kUnloading:
            UnloadImpl(context);
            break;

        default:
            break;
        }
    }


    void EntityRegistry::LoadImpl(const EntityLoadingContext& context)
    {
        FE_Assert(m_state.load(std::memory_order_acquire) == State::kLoading);

        // TODO: entity serialization
        FE_Unused(context);

        m_state.store(State::kLoaded, std::memory_order_release);
    }


    void EntityRegistry::UnloadImpl(const EntityLoadingContext& context)
    {
        FE_Assert(m_state.load(std::memory_order_acquire) == State::kUnloading);

        FE_Unused(context);

        m_state.store(State::kUnloaded, std::memory_order_release);
    }


    void EntityRegistry::Update(const EntityUpdateContext& context)
    {
        Memory::FiberTempAllocator temp;
        SegmentedVector<DeferredActionJob> deferredActionJobs{ &temp };
        SegmentedVector<LocalSystemUpdateJob> localSystemUpdateJobs{ &temp };

        const uint32_t entitySegmentCount = m_entitiesUnsorted.segment_count();
        Entity*** entitySegments = m_entitiesUnsorted.segments();
        for (uint32_t segmentIndex = 0; segmentIndex < entitySegmentCount; ++segmentIndex)
        {
            constexpr uint32_t kEntitiesPerSegment = SegmentedVector<Entity*>::kElementsPerSegment;
            constexpr uint32_t kMaxEntitiesPerJob = 32;

            const uint32_t entityCount =
                Math::Min(kEntitiesPerSegment, m_entitiesUnsorted.size() - segmentIndex * kEntitiesPerSegment);

            Entity** segment = entitySegments[segmentIndex];
            for (uint32_t startIndex = 0; startIndex < entityCount; startIndex += kMaxEntitiesPerJob)
            {
                const uint32_t endIndex = Math::Min(startIndex + kMaxEntitiesPerJob, entityCount);
                const festd::span entities{ segment + startIndex, endIndex - startIndex };

                auto& deferredActionJob = deferredActionJobs.push_back();
                deferredActionJob.m_entities = entities;

                auto& localSystemUpdateJob = localSystemUpdateJobs.push_back();
                localSystemUpdateJob.m_entities = entities;
                localSystemUpdateJob.m_context = &context;
            }
        }

        Rc waitGroup = WaitGroup::Create(deferredActionJobs.size());
        for (DeferredActionJob& job : deferredActionJobs)
            job.ScheduleForeground(context.m_jobSystem, waitGroup.Get());
        waitGroup->Wait();

        m_deferredActionsAllocator.Clear();

        waitGroup = WaitGroup::Create(localSystemUpdateJobs.size());
        for (LocalSystemUpdateJob& job : localSystemUpdateJobs)
            job.ScheduleForeground(context.m_jobSystem, waitGroup.Get());
        waitGroup->Wait();
    }
} // namespace FE::Framework
