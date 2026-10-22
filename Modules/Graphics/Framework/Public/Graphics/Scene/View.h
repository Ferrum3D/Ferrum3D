#pragma once
#include <Core/Env/Environment.h>
#include <Core/Math/Transform.h>
#include <Graphics/Base/Base.h>
#include <Graphics/Base/BaseModuleList.h>
#include <Graphics/Core/Base.h>
#include <Graphics/Core/Texture.h>
#include <festd/span.h>
#include <festd/string.h>

namespace FE::Graphics
{
    //! @brief Published during View::Update by a debug module that replaces the view's lit scene color.
    //!
    //! Passes whose results the replacement discards skip themselves.
    struct ViewColorOverride final
    {
        bool m_needsSurfaceEvaluation = false;
    };


    //! @brief Published by the renderer before output passes are added, once the view's output target is acquired.
    struct ViewOutput final
    {
        Core::TextureView m_target = Core::TextureView::kInvalid;
        //! @brief True when earlier views or modules wrote every pixel of m_target this frame.
        bool m_isInitialized = false;
    };


    //! @brief A rendering feature of one View.
    //!
    //! Update publishes the module's data into the view blackboard. The renderer adds passes in a fixed order;
    //! each pass function skips itself when the data of its module is missing.
    struct ViewModuleBase : public Memory::RefCountedObjectBase
    {
        FE_RTTI("77BC4DE9-A792-4A65-BCB8-F23556551774");

        ~ViewModuleBase() override = default;

        virtual void Update(Core::FrameGraphBlackboard& blackboard) = 0;

        //! @brief Diagnostic visualizations implemented by this module, identified by command-line and UI names.
        [[nodiscard]] virtual festd::span<const Env::Name> GetDebugModes() const
        {
            return {};
        }

        //! @brief Select an entry of GetDebugModes(); kInvalidIndex restores normal output.
        virtual void SetDebugMode(uint32_t) {}

        [[nodiscard]] virtual uint32_t GetDebugMode() const
        {
            return kInvalidIndex;
        }

    protected:
        explicit ViewModuleBase(View* view)
            : m_view(view)
        {
        }

        View* m_view = nullptr;
    };

    using ViewModuleList = BaseModuleList<View, ViewModuleBase>;


    //! @brief A rendering of one Scene; the caller of Renderer::RenderAsync selects the target receiving its output.
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

        [[nodiscard]] const Transform& GetCameraTransform() const
        {
            return m_cameraTransform;
        }

        [[nodiscard]] const Matrix4x4& GetViewMatrix() const
        {
            return m_viewMatrix;
        }

        [[nodiscard]] const Matrix4x4& GetProjectionMatrix() const
        {
            return m_projectionMatrix;
        }

        [[nodiscard]] const Matrix4x4& GetViewProjectionMatrix() const
        {
            return m_viewProjectionMatrix;
        }

        [[nodiscard]] const Matrix4x4& GetInverseViewProjectionMatrix() const
        {
            return m_inverseViewProjectionMatrix;
        }

        //! @brief Enable or disable this view in scene rendering without removing it from its scene.
        void SetEnabled(bool enabled)
        {
            m_enabled = enabled;
        }


        //! @brief Return whether the renderer should process this view.
        [[nodiscard]] bool IsEnabled() const
        {
            return m_enabled;
        }

        void SetCameraTransform(const Transform& transform);
        void SetProjection(float fovY, float aspectRatio, float near, float far);

        //! @brief Enable a diagnostic mode of one of the view's modules and disable all others.
        //!
        //! An invalid name restores normal output. Returns false when no module implements the mode.
        bool SetDebugMode(Env::Name name);

        //! @brief Return the active diagnostic mode, or an invalid name for normal output.
        [[nodiscard]] Env::Name GetDebugMode() const;

        virtual void Update(Core::FrameGraphBlackboard& blackboard) = 0;

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
        bool m_enabled = true;

        Matrix4x4 m_viewMatrix = Matrix4x4::kIdentity;
        Matrix4x4 m_projectionMatrix = Matrix4x4::kIdentity;
        Matrix4x4 m_viewProjectionMatrix = Matrix4x4::kIdentity;
        Matrix4x4 m_inverseViewProjectionMatrix = Matrix4x4::kIdentity;
    };
} // namespace FE::Graphics
