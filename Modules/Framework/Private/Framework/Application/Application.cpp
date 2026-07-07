#include <Core/DI/Builder.h>
#include <Core/IO/BaseIO.h>
#include <Core/Jobs/Jobs.h>
#include <Core/Logging/Logger.h>
#include <Framework/Application/Application.h>
#include <Framework/Application/Core/PlatformEvent.h>

namespace FE::Framework
{
    namespace
    {
        Application* GInstance = nullptr;
    } // namespace


    Application::Application()
    {
        FE_Assert(GInstance == nullptr, "Application already initialized");
        GInstance = this;
    }


    Application::~Application()
    {
        FE_Assert(GInstance == this);
        GInstance = nullptr;
    }


    void Application::InitializeCore()
    {
        DI::ServiceRegistryBuilder builder{ Env::GetRootServiceRegistry() };
        RegisterServices(builder);
        builder.Build();

        Env::Module* module = Env::Module::GetModuleList();
        while (module)
        {
            DI::ServiceRegistryBuilder moduleBuilder{ module->m_serviceRegistry };
            module->RegisterServices(moduleBuilder);
            module = module->m_next;
            moduleBuilder.Build();
        }
    }


    void Application::InitializeWindow()
    {
        DI::IServiceProvider* serviceProvider = Env::GetServiceProvider();
        m_platformApplication = serviceProvider->ResolveRequired<Core::PlatformApplication>();

        Core::PlatformWindowDesc windowDesc;
        windowDesc.m_rect = { 100, 100, 800, 600 };
        windowDesc.m_title = Env::GetApplicationInfo().m_name;
        m_mainWindow = m_platformApplication->CreateWindow(windowDesc);
        m_mainWindow->Show(Core::PlatformWindowShowMode::kMaximized);
    }


    int32_t Application::Run()
    {
        m_exitWaitGroup = WaitGroup::Create();
        m_frameJob.m_application = this;
        m_frameJob.Dispatch(Jobs::FiberAffinityMask::kMainThread, m_exitWaitGroup.Get(), Jobs::Priority::kHigh);
        m_exitWaitGroup->Wait();
        return m_exitCode;
    }


    Application& Application::Get()
    {
        return *GInstance;
    }


    void Application::FrameJob::Execute()
    {
        auto* app = m_application->m_platformApplication.Get();
        if (app == nullptr)
        {
            if (const Rc<WaitGroup> waitGroup = m_application->ScheduleUpdate())
                waitGroup->Wait();

            IO::Flush(IO::StandardDescriptor::kStdout);
            IO::Flush(IO::StandardDescriptor::kStderr);
            return;
        }

        while (!app->IsCloseRequested())
        {
            FrameMark;

            FE_PROFILER_ZONE_NAMED("Frame");
            app->PollEvents();
            if (app->IsCloseRequested())
                break;

            if (const auto waitGroup = m_application->ScheduleUpdate())
                waitGroup->Wait();

            IO::Flush(IO::StandardDescriptor::kStdout);
            IO::Flush(IO::StandardDescriptor::kStderr);
        }

        IO::Flush(IO::StandardDescriptor::kStdout);
        IO::Flush(IO::StandardDescriptor::kStderr);
        Jobs::StopJobSystem();
        m_application->m_exitCode = 0;
    }


    void Application::RegisterServices([[maybe_unused]] const DI::ServiceRegistryBuilder& builder) {}
} // namespace FE::Framework
