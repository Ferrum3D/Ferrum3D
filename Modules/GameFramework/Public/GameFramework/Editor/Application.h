#pragma once
#include <Core/RTTI/Any.h>
#include <GameFramework/Application.h>
#include <GameFramework/Editor/AssetBrowser.h>
#include <GameFramework/Editor/CameraMovementSystem.h>
#include <GameFramework/Editor/Gizmos.h>
#include <GameFramework/Editor/SelectionSystem.h>

namespace FE::Editor
{
    //! Reusable paused world editor. Game-specific tools belong in the game's shared library.
    struct Application : GameFramework::Application
    {
        FE_RTTI("7BE656B8-24BA-48FF-8F35-0AB778C3BCD2");

        bool Initialize(const GameFramework::ApplicationSettings& settings) override;

        [[nodiscard]] Framework::EntityID GetSelection() const
        {
            return m_selected;
        }

        void Select(Framework::EntityID entity)
        {
            m_selected = entity;
        }

        //! Queue a visibility-buffer pick at normalized viewport coordinates.
        bool Pick(Vector2 position);

    protected:
        void PrepareWorldDefinition(Framework::EntityWorldAsset& definition) override;
        void OnWorldLoaded() override;
        void OnWorldUnloading() override;
        void BeforeWorldUpdate() override;
        void ScheduleWorldUpdate(Framework::EntityWorld& world) override;
        void DrawImGuiWindows() override;
        void BuildRenderRequest(Graphics::RenderRequest& request) override;
        virtual void DrawGameEditorMenu() {}
        virtual void DrawGameEditorWindows() {}

    private:
        //! Inspector edits of one component, kept until the submitted replacements are visible in the world.
        struct ComponentEdit final
        {
            Framework::EntityID m_entity;
            const Rtti::Type* m_type = nullptr;
            Rtti::Any m_value;
            int32_t m_lastEditFrame = 0;
            bool m_isPending = false;
        };

        Graphics::View* GetEditorView();
        void SubmitPendingEdits();
        void DrawWorldWindow();
        void DrawEntityWindow();
        void DrawServicesWindow();
        void DrawComponent(Framework::Entity& entity, const Framework::EntityComponentInfo& component);
        void DrawEntity(Framework::Entity& entity, uint32_t depth);
        void DrawViewport();
        void DrawOpenWorld();

        AssetBrowser m_assets;
        ComponentEdit m_componentEdit;
        TranslationGizmo m_gizmo;
        IO::Path m_assetDirectory;
        festd::string m_serviceSettingsError;
        Uuid m_cameraUuid = Uuid::kNull;
        Framework::EntityID m_selected;
        Transform m_initialCamera = Transform::Translation(Vector3(0, 2, -6));
        Rc<Graphics::Core::Texture> m_viewportTexture;
        SelectionSystem m_selection;
        CameraMovementSystem m_cameraMovement;
        Input::ContextHandle m_inputContext;
        bool m_editorSystemsRegistered = false;
        Vector2UInt m_viewportSize{ 1, 1 };
        float m_uiScale = 1.0f;
        bool m_viewportHovered = false;
        bool m_viewportFocused = false;
        bool m_openWorldDialog = false;
        bool m_openFailed = false;
        char m_worldPath[1024]{};
    };
} // namespace FE::Editor
