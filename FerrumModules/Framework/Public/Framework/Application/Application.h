#pragma once
#include <FeCore/Jobs/Job.h>
#include <FeCore/Modules/IModule.h>
#include <Framework/Application/Core/PlatformApplication.h>
#include <Framework/Application/Core/PlatformEvent.h>
#include <Framework/Application/Core/PlatformWindow.h>
#include <Framework/Module.h>

namespace FE::Framework
{
    struct Application : public Memory::RefCountedObjectBase
    {
        FE_RTTI_Class(Application, "AF07EDCA-2D55-4E2D-A5EF-85ED53B4CDAB");

        Application() = default;
        ~Application() override = default;

        Application(const Application&) = delete;
        Application(Application&&) = delete;
        Application& operator=(const Application&) = delete;
        Application& operator=(Application&&) = delete;

        void InitializeCore()
        {
            DI::ServiceRegistryBuilder builder{ Env::GetRootServiceRegistry() };
            DI::RegisterCoreServices(builder);
            RegisterServices(builder);
            builder.Build();

            m_moduleLoadingList.Add<FrameworkModule>();
            LoadModules(m_moduleLoadingList);

            DI::IServiceProvider* serviceProvider = Env::GetServiceProvider();
            m_platformApplication = serviceProvider->ResolveRequired<Core::PlatformApplication>();
            m_jobSystem = serviceProvider->ResolveRequired<IJobSystem>();
        }

        void Initialize()
        {
            InitializeWindow();
            InitializeApp();
        }

        int32_t Run();

    protected:
        struct FrameJob final : public Job
        {
            void Execute() override;

            Application* m_application = nullptr;
        };

        virtual void LoadModules([[maybe_unused]] ModuleLoadingList& modules) {}
        virtual void RegisterServices([[maybe_unused]] DI::ServiceRegistryBuilder builder) {}
        virtual void InitializeApp() {}

        virtual Rc<WaitGroup> ScheduleUpdate() = 0;

        void InitializeWindow()
        {
            Core::PlatformWindowDesc windowDesc;
            windowDesc.m_rect = { 100, 100, 800, 600 };
            windowDesc.m_title = Env::GetApplicationInfo().m_name;
            m_mainWindow = m_platformApplication->CreateWindow(windowDesc);
            m_mainWindow->Show(Core::PlatformWindowShowMode::kMaximized);
        }

        ModuleLoadingList m_moduleLoadingList;
        Rc<Core::PlatformApplication> m_platformApplication;
        Rc<Core::PlatformWindow> m_mainWindow;
        Rc<IJobSystem> m_jobSystem;
        Rc<WaitGroup> m_exitWaitGroup;
        FrameJob m_frameJob;
        int32_t m_exitCode = 0;
    };


    inline int32_t Application::Run()
    {
        m_exitWaitGroup = WaitGroup::Create();
        m_frameJob.m_application = this;
        m_frameJob.Schedule(m_jobSystem.Get(), FiberAffinityMask::kMainThread, m_exitWaitGroup.Get(), JobPriority::kHigh);
        m_exitWaitGroup->Wait();
        return m_exitCode;
    }


    inline void Application::FrameJob::Execute()
    {
        auto* app = m_application->m_platformApplication.Get();
        while (!app->IsCloseRequested())
        {
            FrameMark;

            FE_PROFILER_ZONE_NAMED("Frame");
            app->PollEvents();
            auto waitGroup = m_application->ScheduleUpdate();

            if (waitGroup)
                waitGroup->Wait();

            Console::Flush();
        }
    }
} // namespace FE::Framework
