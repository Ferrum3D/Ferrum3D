#include <Core/Threading/Thread.h>
#include <GameFramework/WorldStreamingService.h>

namespace FE::GameFramework
{
    void WorldStreamingService::Init(Framework::EntityWorld& world)
    {
        FE_Assert(!m_world);
        m_world = &world;
    }


    void WorldStreamingService::Shutdown(Framework::EntityWorld&)
    {
        m_requests.clear();
        m_entries.clear();
        m_world = nullptr;
    }


    void WorldStreamingService::Load(const Uuid registry, const IO::AssetID placement)
    {
        FE_Assert(Threading::IsMainThread() && m_world && registry.IsValid() && placement.IsValid());
        m_requests.push_back({ registry, placement });
    }


    void WorldStreamingService::Unload(const Uuid registry)
    {
        FE_Assert(Threading::IsMainThread() && m_world && registry.IsValid());
        m_requests.push_back({ registry, IO::AssetID::kNull });
    }


    Framework::MaterializationStatus WorldStreamingService::GetStatus(const IO::AssetID placement) const
    {
        FE_Assert(Threading::IsMainThread() && m_world);
        for (const auto& entry : m_entries)
        {
            if (entry.m_placement == placement)
                return m_world->GetMaterializationStatus(entry.m_token);
        }

        return { Framework::MaterializationState::kFailed, {}, "Placement was not requested" };
    }


    void WorldStreamingService::Update(Framework::EntityWorld& world)
    {
        FE_PROFILER_ZONE();
        for (const auto& request : m_requests)
        {
            auto* registry = world.FindRegistry(request.m_registry);
            if (!request.m_placement.IsValid())
            {
                if (registry)
                    world.RemoveRegistry(*registry);

                continue;
            }

            if (!registry)
                registry = &world.CreateRegistry(request.m_registry);

            const auto token = world.LoadPlacement(*registry, request.m_placement);
            auto found = festd::find_if(m_entries.begin(), m_entries.end(), [&](const Entry& entry) {
                return entry.m_placement == request.m_placement;
            });
            if (found == m_entries.end())
                m_entries.push_back({ request.m_placement, token });
            else
                found->m_token = token;
        }

        m_requests.clear();
    }
} // namespace FE::GameFramework
