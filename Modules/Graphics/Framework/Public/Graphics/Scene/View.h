#pragma once
#include <FeCore/Math/Transform.h>
#include <Graphics/Base/Base.h>
#include <Graphics/Base/BaseModuleList.h>
#include <Graphics/Core/Base.h>

namespace FE::Graphics
{
    struct IViewModule : public Memory::RefCountedObjectBase
    {
        FE_RTTI("77BC4DE9-A792-4A65-BCB8-F23556551774");

        ~IViewModule() override = default;

        virtual void Update(Core::FrameGraphBlackboard& blackboard) = 0;
    };

    using ViewModuleList = BaseModuleList<View, IViewModule>;


    struct View : public Memory::RefCountedObjectBase
    {
        FE_RTTI("4B83AFA4-70B9-4AF9-8AB8-1082C6B49849");

        ~View() override = default;

        [[nodiscard]] Scene* GetScene() const
        {
            return m_scene;
        }

        [[nodiscard]] ViewModuleList& GetModules()
        {
            return m_moduleList;
        }

        [[nodiscard]] const ViewModuleList& GetModules() const
        {
            return m_moduleList;
        }

        void SetCameraTransform(const Transform& transform);
        void SetProjection(float fovY, float aspectRatio, float near, float far);

    protected:
        View(Scene* scene)
            : m_scene(scene)
            , m_moduleList(this)
        {
        }

        Scene* m_scene = nullptr;
        ViewModuleList m_moduleList;

        Transform m_cameraTransform{ kForceInit };
        Transform m_inverseCameraTransform{ kForceInit };

        float m_fovY = 1.0f;
        float m_aspectRatio = 1.0f;
        float m_nearPlane = 1.0f;
        float m_farPlane = 100.0f;

        Matrix4x4 m_viewMatrix = Matrix4x4::Identity();
        Matrix4x4 m_projectionMatrix = Matrix4x4::Identity();
        Matrix4x4 m_viewProjectionMatrix = Matrix4x4::Identity();
        Matrix4x4 m_inverseViewProjectionMatrix = Matrix4x4::Identity();
    };
} // namespace FE::Graphics
