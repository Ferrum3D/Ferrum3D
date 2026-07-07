#pragma once
#include <Core/DI/BaseDI.h>
#include <Core/Jobs/JobNode.h>
#include <Core/Logging/Logger.h>
#include <Framework/Application/Core/PlatformApplication.h>
#include <Framework/Application/Core/PlatformWindow.h>
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

        void InitializeCore();
        void InitializeWindow();

        int32_t Run();

        static Application& Get();

    protected:
        struct FrameJob final : public Jobs::JobNode
        {
            void Execute() override;

            Application* m_application = nullptr;
        };

        virtual void RegisterServices(const DI::ServiceRegistryBuilder& builder);

        virtual Rc<WaitGroup> ScheduleUpdate() = 0;

        festd::unordered_dense_map<festd::string_view, uint32_t> m_commandLineArguments;

        Rc<Core::PlatformApplication> m_platformApplication;
        Rc<Core::PlatformWindow> m_mainWindow;
        Rc<WaitGroup> m_exitWaitGroup;
        FrameJob m_frameJob;
        int32_t m_exitCode = 0;
    };
} // namespace FE::Framework
