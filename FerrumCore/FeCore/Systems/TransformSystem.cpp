#include <FeCore/Systems/TransformSystem.h>

namespace FE::ECS
{
    void TransformSystem::OnUpdate(const FrameEventArgs& args)
    {
        m_Query->Update();
        m_Query->ForEach(std::function([](LocalToWorldComponent& localToWorld, const Position3DComponent& pos) {
            localToWorld.Matrix = Matrix4x4F::CreateTranslation(pos.AsVector3F());
        }));
    }

    void TransformSystem::OnCreate()
    {
        m_Query = MakeShared<EntityQuery>(Registry());
        m_Query->AllOf<LocalToWorldComponent, Position3DComponent>();
    }
} // namespace FE::ECS
