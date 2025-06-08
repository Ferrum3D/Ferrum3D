#pragma once
#include <FeCore/Jobs/Job.h>
#include <FeCore/Modules/IModule.h>
#include <Framework/Application/Core/PlatformApplication.h>
#include <Framework/Application/Core/PlatformEvent.h>
#include <Framework/Application/Core/PlatformWindow.h>
#include <Framework/Module.h>

namespace FE::Framework
{
    struct LogColorScope final
    {
        explicit LogColorScope(const LogSeverity severity)
        {
            Console::Color color;
            switch (severity)
            {
            default:
                FE_DebugBreak();
                [[fallthrough]];

            case LogSeverity::kWarning:
                color = Console::Color::kYellow;
                break;

            case LogSeverity::kError:
            case LogSeverity::kCritical:
                color = Console::Color::kRed;
                break;

            case LogSeverity::kTrace:
            case LogSeverity::kDebug:
                color = Console::Color::kAqua;
                break;

            case LogSeverity::kInfo:
                color = Console::Color::kLime;
                break;
            }

            Console::SetTextColor(color);
        }

        ~LogColorScope()
        {
            Console::SetTextColor(Console::Color::kDefault);
        }
    };


    struct StdoutLogSink final : public LogSinkBase
    {
        StdoutLogSink(Logger* logger)
            : LogSinkBase(logger)
        {
        }

        void Log(const LogSeverity severity, const SourceLocation sourceLocation, const festd::string_view message) override
        {
            {
                const auto location = Fmt::FixedFormatSized<IO::kMaxPathLength + 16>(
                    "{}({}): ", sourceLocation.m_fileName, sourceLocation.m_lineNumber);
                Console::Write(festd::string_view(location.data(), location.size()));
            }

            {
                const auto date = DateTime<TZ::Local>::Now().ToString(DateTimeFormatKind::kISO8601);
                Console::Write(date);
            }

            Console::Write(" [");

            {
                LogColorScope colorScope{ severity };
                Console::Write(LogSeverityToString(severity));
            }

            Console::Write("] ");

            Console::Write(festd::string_view(message.data(), message.size()));
            Console::Write("\n");

            if (severity >= LogSeverity::kWarning)
                Console::Flush();
        }
    };


    struct Application : public Memory::RefCountedObjectBase
    {
        FE_RTTI_Class(Application, "AF07EDCA-2D55-4E2D-A5EF-85ED53B4CDAB");

        Application(const int32_t argc, const char** argv)
        {
            for (int32_t i = 0; i < argc; ++i)
                m_commandLine.push_back(argv[i]);
        }

        ~Application() override = default;

        Application(const Application&) = delete;
        Application(Application&&) = delete;
        Application& operator=(const Application&) = delete;
        Application& operator=(Application&&) = delete;

        void InitializeCore()
        {
            DI::ServiceRegistryBuilder builder{ Env::GetRootServiceRegistry() };
            DI::RegisterCoreServices(builder);

            builder.Bind<Env::Configuration>()
                .ToFunc([this](DI::IServiceProvider*, Memory::RefCountedObjectBase** result) {
                    std::pmr::memory_resource* allocator = Env::GetStaticAllocator(Memory::StaticAllocatorType::kLinear);
                    *result = Rc<Env::Configuration>::New(allocator, m_commandLine);
                    return DI::ResultCode::kSuccess;
                })
                .InSingletonScope();

            RegisterServices(builder);
            builder.Build();

            m_moduleLoadingList.Add<FrameworkModule>();
            LoadModules(m_moduleLoadingList);

            DI::IServiceProvider* serviceProvider = Env::GetServiceProvider();
            m_jobSystem = serviceProvider->ResolveRequired<IJobSystem>();
        }

        void InitializeWindow()
        {
            DI::IServiceProvider* serviceProvider = Env::GetServiceProvider();
            m_platformApplication = serviceProvider->ResolveRequired<Core::PlatformApplication>();

            Core::PlatformWindowDesc windowDesc;
            windowDesc.m_rect = { 100, 100, 800, 600 };
            windowDesc.m_title = Env::GetApplicationInfo().m_name;
            m_mainWindow = m_platformApplication->CreateWindow(windowDesc);
            m_mainWindow->Show(Core::PlatformWindowShowMode::kMaximized);
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

        virtual Rc<WaitGroup> ScheduleUpdate() = 0;

        festd::vector<festd::string_view> m_commandLine;
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
        if (app == nullptr)
        {
            auto waitGroup = m_application->ScheduleUpdate();

            if (waitGroup)
                waitGroup->Wait();

            Console::Flush();
            return;
        }

        while (!app->IsCloseRequested())
        {
            FrameMark;

            FE_PROFILER_ZONE_NAMED("Frame");
            app->PollEvents();
            if (app->IsCloseRequested())
                break;

            auto waitGroup = m_application->ScheduleUpdate();

            if (waitGroup)
                waitGroup->Wait();

            Console::Flush();
        }
    }
} // namespace FE::Framework
