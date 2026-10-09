#include <Framework/Entities/EntityWorld.h>
#include <GameFramework/WorldGraphicsSceneService.h>
#include <cmath>

namespace FE::GameFramework
{
    bool CameraComponent::Validate() const
    {
        if (!std::isfinite(m_fovY) || m_fovY <= 0 || m_fovY >= Constants::kPI)
            return false;

        if (!std::isfinite(m_aspectRatio) || m_aspectRatio <= 0)
            return false;

        if (!std::isfinite(m_nearPlane) || m_nearPlane <= 0)
            return false;

        return std::isfinite(m_farPlane) && m_farPlane > m_nearPlane;
    }


    Framework::LifecycleResult MeshRuntimeComponent::Activate(Framework::ComponentContext& context)
    {
        m_scene = context.m_world.FindService<WorldGraphicsSceneService>();
        if (!m_scene)
            return Framework::LifecycleResult::kFailed;

        const auto* mesh = context.m_entity.FindComponent<const MeshComponent>();
        FE_Assert(mesh);
        return m_scene->GetMeshes().Create(*mesh, *this);
    }


    void MeshRuntimeComponent::Deactivate(Framework::ComponentContext&)
    {
        if (m_scene)
            m_scene->GetMeshes().Destroy(*this);

        m_scene = nullptr;
    }


    Framework::LifecycleResult CameraRuntimeComponent::Init(Framework::ComponentContext& context)
    {
        m_scene = context.m_world.FindService<WorldGraphicsSceneService>();
        const auto* camera = context.m_entity.FindComponent<const CameraComponent>();
        FE_Assert(camera);
        if (!m_scene || !camera->Validate())
            return Framework::LifecycleResult::kFailed;

        m_view = m_scene->CreateCameraView();
        return Framework::LifecycleResult::kSucceeded;
    }


    void CameraRuntimeComponent::Shutdown(Framework::ComponentContext&)
    {
        if (m_view)
        {
            m_scene->GetScene().DestroyView(m_view.Get());
            m_view.Reset();
        }

        m_scene = nullptr;
    }


    Framework::LifecycleResult CameraRuntimeComponent::Activate(Framework::ComponentContext&)
    {
        FE_Assert(m_view);
        m_view->SetEnabled(true);
        return Framework::LifecycleResult::kSucceeded;
    }


    void CameraRuntimeComponent::Deactivate(Framework::ComponentContext&)
    {
        if (m_view)
            m_view->SetEnabled(false);
    }
} // namespace FE::GameFramework
