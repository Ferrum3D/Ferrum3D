#pragma once
#include <Framework/Entities/EntityComponentRegistry.h>
#include <Graphics/Features/Mesh/MeshSceneModule.h>
#include <Graphics/Scene/View.h>

namespace FE::GameFramework
{
    struct WorldGraphicsSceneService;


    //! @brief Authored model and material; resolved meshes and scene membership live in a transient companion.
    struct MeshComponent final
    {
        //! @brief Hard model dependency whose mesh products supply geometry.
        IO::Link<Graphics::ModelAsset> m_model;
        //! @brief Hard material dependency shared by this model's instances.
        IO::Link<Graphics::MaterialInstanceAsset> m_material;
        FE_RTTI_Reflect("aaa69126-4427-4055-b1c4-04013aae1001");
        FE_RTTI_Serialize();
    };


    //! @brief Perspective camera settings. Cameras use rigid world transforms.
    struct CameraComponent final
    {
        //! @brief Vertical field of view in radians.
        float m_fovY = Constants::kPI * 0.3f;
        //! @brief Projection width divided by height.
        float m_aspectRatio = 16.0f / 9.0f;
        //! @brief Positive near clip distance.
        float m_nearPlane = 0.01f;
        //! @brief Far clip distance, greater than the near plane.
        float m_farPlane = 1000.0f;
        //! @brief Reject invalid perspective settings without asserting on asset content.
        [[nodiscard]] bool Validate() const;
        FE_RTTI_Reflect("aaa69126-4427-4055-b1c4-04013aae1002");
        FE_RTTI_Serialize();
    };


    //! @brief Relocatable transient scene membership; lifecycle removes it before asset residency is released.
    struct MeshRuntimeComponent final
    {
        //! @brief Borrowed world scene service used to undo runtime membership.
        WorldGraphicsSceneService* m_scene = nullptr;
        //! @brief Owned scene batch released on deactivation.
        Graphics::MeshBatch* m_batch = nullptr;
        //! @brief Generation-checked scene instances; relocation transfers their ownership.
        festd::vector<Graphics::MeshHandle> m_handles;
        //! @brief Last submitted model identity.
        IO::AssetID m_model = IO::AssetID::kNull;
        //! @brief Last submitted material identity.
        IO::AssetID m_material = IO::AssetID::kNull;
        FE_RTTI_Reflect("aaa69126-4427-4055-b1c4-04013aae1003");
        //! @brief Create scene membership after hard dependencies are ready.
        Framework::LifecycleResult Activate(Framework::ComponentContext& context);
        //! @brief Release scene membership before component residency is released.
        void Deactivate(Framework::ComponentContext& context);
    };


    //! @brief Relocatable transient camera View ownership; each camera creates its own View.
    struct CameraRuntimeComponent final
    {
        //! @brief View created during initialization and retained until shutdown.
        Rc<Graphics::View> m_view;
        //! @brief Borrowed world scene service, which outlives this component.
        WorldGraphicsSceneService* m_scene = nullptr;
        FE_RTTI_Reflect("aaa69126-4427-4055-b1c4-04013aae1004");
        //! @brief Create this camera's View after settings and the world graphics service are validated.
        Framework::LifecycleResult Init(Framework::ComponentContext& context);
        //! @brief Remove the View from the scene and release it.
        void Shutdown(Framework::ComponentContext& context);
        //! @brief Enable the initialized View for this camera.
        Framework::LifecycleResult Activate(Framework::ComponentContext& context);
        //! @brief Disable the View while the camera is inactive.
        void Deactivate(Framework::ComponentContext& context);
    };
} // namespace FE::GameFramework
