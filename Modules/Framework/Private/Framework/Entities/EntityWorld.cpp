#include <FeCore/Jobs/IJobSystem.h>
#include <FeCore/Memory/FiberTempAllocator.h>
#include <FeCore/Memory/PoolAllocator.h>
#include <Framework/Entities/EntityRegistry.h>
#include <Framework/Entities/EntityWorld.h>
#include <Framework/Entities/EntityWorldSystem.h>

namespace FE::Framework
{
    namespace
    {
        Memory::SpinLockedPoolAllocator GEntityRegistryPool{ "EntityRegistryPool", sizeof(EntityRegistry) };

        Threading::SpinLock GEntityWorldListLock;
        uint32_t GFreeEntityWorldIDs = (1u << kInvalidEntityWorldID) - 1;
        festd::intrusive_list<EntityWorld> GEntityWorldList;
    } // namespace


    EntityWorld::ListReader::ListReader(festd::intrusive_list<EntityWorld>* list)
    {
        GEntityWorldListLock.lock();
        m_list = list;
    }


    EntityWorld::ListReader::~ListReader()
    {
        GEntityWorldListLock.unlock();
    }


    EntityWorld::ListReader::ListReader(ListReader&& other) noexcept
    {
        m_list = other.m_list;
        other.m_list = nullptr;
    }


    EntityWorld::ListReader& EntityWorld::ListReader::operator=(ListReader&& other) noexcept
    {
        festd::swap(m_list, other.m_list);
        return *this;
    }


    EntityWorld::ListReader EntityWorld::GetWorldList()
    {
        return ListReader(&GEntityWorldList);
    }


    EntityRegistry* EntityWorld::CreateRegistry()
    {
        std::lock_guard lock{ m_lock };

        auto* registry = Memory::New<EntityRegistry>(&GEntityRegistryPool, this);
        m_registries.push_back(registry);

        if (m_freeRegistryIDs.size() < m_registries.capacity())
            m_freeRegistryIDs.resize(m_registries.capacity(), true);

        const uint32_t id = m_freeRegistryIDs.find_first();
        m_freeRegistryIDs.reset(id);

        registry->m_ID = id;
        registry->RequestLoad();

        return registry;
    }


    void EntityWorld::AddSystem(EntityWorldSystem* system)
    {
        std::lock_guard lock{ m_lock };
        FE_Assert(!m_isUpdating);
        m_worldSystems.push_back(system);
        system->Init();
    }


    void EntityWorld::RemoveSystem(EntityWorldSystem* system)
    {
        std::lock_guard lock{ m_lock };
        FE_Assert(!m_isUpdating);

        const auto it = festd::find(m_worldSystems, system);
        FE_Assert(it != m_worldSystems.end());
        m_worldSystems.erase_unsorted(it);

        system->Shutdown();
        system->Destroy();
    }


    void EntityWorld::UpdateLoadingState()
    {
        std::lock_guard lock{ m_lock };

        for (uint32_t registryIndex = 0; registryIndex < m_registries.size();)
        {
            EntityRegistry* registry = m_registries[registryIndex];
            registry->UpdateLoadingState(m_loadingContext);

            switch (registry->GetState())
            {
            case EntityRegistry::State::kUnloaded:
                Memory::Delete(&GEntityRegistryPool, registry, sizeof(EntityRegistry));
                festd::swap(m_registries[registryIndex], m_registries.back());
                m_registries.pop_back();
                break;

            default:
                ++registryIndex;
                break;
            }
        }
    }


    void EntityWorld::Update()
    {
        m_isUpdating = true;
        auto restoreUpdating = festd::defer([this] {
            m_isUpdating = false;
        });

        UpdateLoadingState();

        Memory::FiberTempAllocator temp;
        SegmentedVector<const Archetype*> newArchetypes{ &temp };

        for (EntityRegistry* registry : m_registries)
        {
            registry->Update(m_updateContext);
            for (uint32_t archetypeIndex = registry->m_prevArchetypeCount; archetypeIndex < registry->m_archetypes.size();
                 ++archetypeIndex)
            {
                newArchetypes.push_back(registry->m_archetypes[archetypeIndex]);
            }

            registry->m_prevArchetypeCount = registry->m_archetypes.size();
        }

        for (EntityWorldSystem* system : m_worldSystems)
        {
            for (const Archetype* archetype : newArchetypes)
                system->RegisterArchetype(archetype);

            system->Update(m_updateContext);
        }
    }


    EntityWorld::~EntityWorld()
    {
        FE_Assert(m_registries.size() == 1);

        EntityRegistry* persistentRegistry = m_registries[0];
        Memory::Delete(&GEntityRegistryPool, persistentRegistry, sizeof(EntityRegistry));
        m_registries.clear();

        for (EntityWorldSystem* system : m_worldSystems)
        {
            system->Shutdown();
            system->Destroy();
        }
        m_worldSystems.clear();

        std::lock_guard lock{ GEntityWorldListLock };
        festd::intrusive_list<EntityWorld>::remove(*this);
        GFreeEntityWorldIDs |= (UINT32_C(1) << m_ID);
    }


    EntityWorld::EntityWorld()
    {
        std::lock_guard lock{ GEntityWorldListLock };
        GEntityWorldList.push_back(*this);

        Bit::ScanForward(m_ID, GFreeEntityWorldIDs);
        FE_Assert(m_ID < kInvalidEntityWorldID);
        GFreeEntityWorldIDs &= ~(UINT32_C(1) << m_ID);

        DI::IServiceProvider* serviceProvider = Env::GetServiceProvider();
        IJobSystem* jobSystem = serviceProvider->ResolveRequired<IJobSystem>();

        m_loadingContext.Initialize(jobSystem);
        m_updateContext.Initialize(jobSystem);

        // Create persistent registry
        FE_Unused(CreateRegistry());
    }
} // namespace FE::Framework
