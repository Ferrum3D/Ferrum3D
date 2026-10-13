#include <Framework/Entities/Query.h>
#include <Framework/Entities/WorldTimeService.h>
#include <GameFramework/Components/MeshComponent.h>
#include <GameFramework/Components/TransformComponents.h>
#include <GameFramework/Phases.h>
#include <GameFramework/Systems/TestRotationSystem.h>

namespace FE::GameFramework
{
    void TestRotationSystem::Update(Framework::EntityUpdateContext& context)
    {
        FE_PROFILER_ZONE();

        const auto* time = context.m_world.FindService<Framework::WorldTimeService>();
        FE_Assert(time != nullptr, "Test rotation requires WorldTimeService");

        const float deltaSeconds = time->GetDeltaSeconds();
        if (deltaSeconds <= 0)
            return;

        const Quaternion rotationDelta = Quaternion::RotationY(deltaSeconds);
        const auto traverse = [rotationDelta](const MeshComponent&, TransformComponent& transform) {
            const Quaternion rotation = Math::Normalize(rotationDelta * transform.m_local.Rotation());
            transform.m_local = Transform::Create(transform.m_local.Translation(), rotation, transform.m_local.Scale());
            return Framework::QueryWriteResult::kChanged;
        };

        using Query = Framework::Query<const MeshComponent, TransformComponent>;
        Query::Traverse(context, Phases::Input, Framework::ExecutionPolicy::kParallel, traverse);
    }
} // namespace FE::GameFramework
