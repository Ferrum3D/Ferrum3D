#include <FeCore/Console/FeLog.h>
#include <FeCore/ECS/ISystem.h>
#include <FeCore/ECS/World.h>

namespace FE::ECS
{
    World::World()
    {
        m_Registry = MakeShared<EntityRegistry>();
    }

    void World::RegisterSystem(ISystem* system)
    {
        FE_ASSERT(m_Systems.IndexOf(system) == -1);
        m_Systems.Push(system)->OnCreate();
    }

    void World::UnregisterSystem(ISystem* system)
    {
        system->OnDestroy();
        m_Systems.SwapRemove(system);
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
        return m_Registry.GetRaw();
    }
} // namespace FE::ECS
