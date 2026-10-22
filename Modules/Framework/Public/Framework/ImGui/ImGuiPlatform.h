#pragma once
#include <Framework/Application/Core/PlatformApplication.h>
#include <Framework/Application/Core/PlatformWindow.h>

struct ImGuiContext;
struct ImGuiViewport;

namespace FE::Framework
{
    //! @brief Dear ImGui context and platform backend implemented on PlatformApplication and PlatformWindow.
    //!
    //! It feeds platform events and window state to ImGui and owns the platform windows of secondary viewports.
    //! A renderer backend must be installed before the first frame and destroyed before this object.
    struct ImGuiPlatform final
    {
        ImGuiPlatform(Core::PlatformApplication* application, Core::PlatformWindow* mainWindow);
        ~ImGuiPlatform();

        ImGuiPlatform(const ImGuiPlatform&) = delete;
        ImGuiPlatform& operator=(const ImGuiPlatform&) = delete;

        //! @brief Translate events of the latest PlatformApplication::PollEvents and begin an ImGui frame.
        void NewFrame();

        //! @brief Render the ImGui frame, then create, update and destroy the platform windows of its viewports.
        void EndFrame();

    private:
        struct ViewportData;

        static ImGuiPlatform& Get();
        static Core::PlatformWindow* GetWindow(ImGuiViewport* viewport);

        static void CreateViewportWindow(ImGuiViewport* viewport);
        static void DestroyViewportWindow(ImGuiViewport* viewport);
        static void ShowViewportWindow(ImGuiViewport* viewport);
        static void UpdateViewportWindow(ImGuiViewport* viewport);

        void ProcessEvents();
        void UpdateMonitors();
        void UpdateCursor();

        Core::PlatformApplication* m_application = nullptr;
        Rc<Core::PlatformWindow> m_mainWindow;
        ImGuiContext* m_context = nullptr;
        uint64_t m_previousFrameTicks = 0;
        bool m_areMonitorsDirty = true;
    };
} // namespace FE::Framework
