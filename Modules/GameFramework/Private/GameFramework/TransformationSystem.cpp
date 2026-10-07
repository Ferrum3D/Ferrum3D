#include <GameFramework/TransformationSystem.h>

namespace FE::GameFramework
{
    void TransformationSystem::Init(Framework::EntityWorld& world)
    {
        world.Components().Register<TransformComponent>();
        world.Components().Register<NonUniformScaleComponent>();
        world.Components().Register<WorldTransformComponent>({}, { true });
        m_changes = {};
    }


    void TransformationSystem::Update(Framework::EntityUpdateContext& context)
    {
        using TransformQuery = Framework::CascadeQuery<const TransformComponent,
                                                       const NonUniformScaleComponent*,
                                                       Framework::Parent<const WorldTransformComponent*>,
                                                       WorldTransformComponent>;
        m_completion = TransformQuery::TraverseChanged(
            context,
            Phases::Transformation,
            m_changes,
            [](const TransformComponent& local,
               const NonUniformScaleComponent* scale,
               const WorldTransformComponent* parent,
               WorldTransformComponent& output) {
                // Row-vector convention: apply scale, then authored affine local, then parent world.
                const Matrix4x4 effectiveLocal = scale ? Matrix4x4::Scale(scale->m_scale) * local.m_local : local.m_local;
                output.m_world = parent ? effectiveLocal * parent->m_world : effectiveLocal;
            },
            Framework::ExecutionPolicy::kParallelHierarchyTrees);
    }
} // namespace FE::GameFramework
