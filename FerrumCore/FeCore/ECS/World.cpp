#include <FeCore/Console/FeLog.h>
#include <FeCore/ECS/ISystem.h>
#include <FeCore/ECS/World.h>

namespace FE::ECS
{
    World::World()
    {
        m_Registry = Rc<EntityRegistry>::DefaultNew();
    }

    void World::RegisterSystem(ISystem* system)
    {
        FE_ASSERT_MSG(eastl::find(m_Systems.begin(), m_Systems.end(), system) == m_Systems.end(),
                      "System was already registered");
        m_Systems.push_back(system);
        m_Systems.back()->OnCreate();
    }

    void World::UnregisterSystem(ISystem* system)
    {
        system->OnDestroy();
        m_Systems.erase_unsorted(eastl::find(m_Systems.begin(), m_Systems.end(), system));
    }

    void World::OnUpdate(const FrameEventArgs& args)
    {
        for (auto& system : m_Systems)
        {
            system->OnUpdate(args);
        }
    }

    EntityRegistry* World::Registry()
    {
        return m_Registry.Get();
    }
} // namespace FE::ECS
