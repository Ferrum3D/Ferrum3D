#include <Framework/Entities/EntityWorld.h>
#include <GameFramework/Components/CameraComponent.h>
#include <GameFramework/Services/WorldGraphicsSceneService.h>
#include <cmath>

namespace FE::GameFramework
{
    CameraComponent::CameraComponent(const CameraComponent& other)
        : m_fovY(other.m_fovY)
        , m_aspectRatio(other.m_aspectRatio)
        , m_nearPlane(other.m_nearPlane)
        , m_farPlane(other.m_farPlane)
    {
    }


    Framework::LifecycleResult CameraComponent::Init(Framework::ComponentContext& context)
    {
        m_scene = context.m_world.FindService<WorldGraphicsSceneService>();
        if (!m_scene)
        {
            Logger::LogError("No graphics scene found, CameraComponent cannot be initialized");
            return Framework::LifecycleResult::kFailed;
        }

        m_view = m_scene->CreateCameraView();
        return Framework::LifecycleResult::kSucceeded;
    }


    void CameraComponent::Shutdown(Framework::ComponentContext&)
    {
        if (m_view)
        {
            m_scene->GetScene().DestroyView(m_view.Get());
            m_view.Reset();
        }

        m_scene = nullptr;
    }


    Framework::LifecycleResult CameraComponent::Activate(Framework::ComponentContext&)
    {
        FE_Assert(m_view);
        m_view->SetEnabled(true);
        return Framework::LifecycleResult::kSucceeded;
    }


    void CameraComponent::Deactivate(Framework::ComponentContext&)
    {
        if (m_view)
            m_view->SetEnabled(false);
    }
} // namespace FE::GameFramework
