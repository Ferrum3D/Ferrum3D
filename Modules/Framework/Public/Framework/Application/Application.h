#pragma once
#include <Core/Jobs/JobNode.h>
#include <Core/Logging/Logger.h>
#include <Framework/Application/Core/PlatformApplication.h>
#include <Framework/Application/Core/PlatformWindow.h>
#include <Framework/ImGui/ImGuiPlatform.h>
#include <festd/unordered_map.h>
#include <festd/vector.h>

namespace FE::Framework
{
    struct Application : public Memory::RefCountedObjectBase
    {
        FE_RTTI("AF07EDCA-2D55-4E2D-A5EF-85ED53B4CDAB");

        Application();
        ~Application() override;

        Application(const Application&) = delete;
        Application(Application&&) = delete;
        Application& operator=(const Application&) = delete;
        Application& operator=(Application&&) = delete;

        void InitializeWindow();

        //! @brief Create the Dear ImGui context and platform backend for the main window.
        void InitializeImGui();
        //! @brief Destroy the ImGui context; renderer backends must be shut down first.
        void ShutdownImGui();

        int32_t Run();

        static Application& Get();

    protected:
        struct FrameJob final : public Jobs::JobNode
        {
            void Execute() override;

            Application* m_application = nullptr;
        };

        virtual Rc<WaitGroup> ScheduleUpdate() = 0;

        //! @brief Draw the application's ImGui windows; called between the start and the end of each ImGui frame.
        virtual void DrawImGuiWindows() {}

        //! @brief Run one ImGui frame for the latest polled events; its draw data stays valid until the next call.
        void UpdateImGui();

        festd::unordered_dense_map<festd::string_view, uint32_t> m_commandLineArguments;

        Rc<Core::PlatformApplication> m_platformApplication;
        Rc<Core::PlatformWindow> m_mainWindow;
        festd::unique_ptr<ImGuiPlatform> m_imgui;
        Rc<WaitGroup> m_exitWaitGroup;
        FrameJob m_frameJob;
        int32_t m_exitCode = 0;
    };
} // namespace FE::Framework
