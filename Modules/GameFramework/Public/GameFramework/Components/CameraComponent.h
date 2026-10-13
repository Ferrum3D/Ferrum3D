#pragma once
#include <Framework/Entities/EntityComponentRegistry.h>
#include <Graphics/Scene/View.h>

namespace FE::GameFramework
{
    struct WorldGraphicsSceneService;


    //! @brief Perspective settings; copying and serialization exclude camera View ownership.
    struct CameraComponent final
    {
        //! @brief View created during initialization and retained until shutdown.
        FE_SKIP_SERIALIZING Rc<Graphics::View> m_view;
        //! @brief Borrowed world scene service, which outlives this component.
        FE_SKIP_SERIALIZING WorldGraphicsSceneService* m_scene = nullptr;

        //! @brief Vertical field of view in radians.
        float m_fovY = Constants::kPI * 0.3f;
        //! @brief Projection width divided by height.
        float m_aspectRatio = 16.0f / 9.0f;
        //! @brief Positive near clip distance.
        float m_nearPlane = 0.01f;
        //! @brief Far clip distance, greater than the near plane.
        float m_farPlane = 1000.0f;

        CameraComponent() = default;
        CameraComponent(const CameraComponent& other);
        CameraComponent(CameraComponent&&) noexcept = default;

        Framework::LifecycleResult Init(Framework::ComponentContext& context);
        void Shutdown(Framework::ComponentContext& context);

        Framework::LifecycleResult Activate(Framework::ComponentContext& context);
        void Deactivate(Framework::ComponentContext& context);

        FE_RTTI_Reflect("4696C0B3-FFE4-4E66-9D41-BED9E7FEDAD6");
        FE_RTTI_Serialize();
    };
} // namespace FE::GameFramework
