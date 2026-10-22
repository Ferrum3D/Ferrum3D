#pragma once
#include <Graphics/Renderer.h>

struct ImGuiViewport;

namespace FE::Graphics
{
    struct ImGuiSceneModule;


    //! @brief Dear ImGui renderer backend: a UI scene with one View and presentation Viewport per ImGui viewport.
    //!
    //! Create it on the main thread after the platform backend set up the current ImGui context and before the first
    //! ImGui frame. Secondary viewports require platform windows exposing their native handle as PlatformHandleRaw.
    struct ImGuiRenderer final
    {
        ImGuiRenderer(Core::Device* device, Core::Viewport* mainViewport);
        ~ImGuiRenderer();

        ImGuiRenderer(const ImGuiRenderer&) = delete;
        ImGuiRenderer& operator=(const ImGuiRenderer&) = delete;

        //! @brief Publish the finished ImGui frame; call after ImGui::Render and ImGui::UpdatePlatformWindows.
        //!
        //! The renderer must not read the scene concurrently: call after Renderer::WaitForScene.
        void Submit();

        //! @brief Append the UI view of every rendered ImGui viewport, drawn over earlier views of the same viewport.
        void AddViews(RenderRequest& request) const;

        [[nodiscard]] Scene& GetScene() const
        {
            return *m_scene;
        }

    private:
        struct ViewportData;

        static void CreateViewportWindow(ImGuiViewport* viewport);
        static void DestroyViewportWindow(ImGuiViewport* viewport);
        static ImGuiRenderer& Get();

        ViewportData* CreateViewportData(const ImGuiViewport& viewport, Core::Viewport* target);

        Rc<Core::Device> m_device;
        Rc<Scene> m_scene;
        ImGuiSceneModule* m_module = nullptr;
    };
} // namespace FE::Graphics
