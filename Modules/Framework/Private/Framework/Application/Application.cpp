#include <FeCore/Console/Console.h>
#include <FeCore/DI/Builder.h>
#include <FeCore/Logging/Logger.h>
#include <FeCore/Modules/Configuration.h>
#include <FeCore/Time/DateTime.h>
#include <Framework/Application/Application.h>
#include <Framework/Application/Core/PlatformEvent.h>

namespace FE::Framework
{
    namespace
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


        Application* GInstance = nullptr;
    } // namespace


    void StdoutLogSink::Log(const LogSeverity severity, const SourceLocation sourceLocation, const festd::string_view message)
    {
        {
            const auto location = Fmt::FixedFormatSized<512>("{}({}): ", sourceLocation.m_fileName, sourceLocation.m_lineNumber);
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
        Console::Flush();
    }


    Application::Application(const int32_t argc, const char** argv)
    {
        FE_Assert(GInstance == nullptr, "Application already initialized");
        GInstance = this;

        for (int32_t i = 1; i < argc; ++i)
        {
            const festd::string_view arg{ argv[i] };
            m_commandLine.push_back(arg);

            if (arg.starts_with("-"))
                m_commandLineArguments[arg] = i;
        }
    }


    Application::~Application()
    {
        FE_Assert(GInstance == this);
        GInstance = nullptr;
    }


    void Application::InitializeCore()
    {
        DI::ServiceRegistryBuilder builder{ Env::GetRootServiceRegistry() };

        builder.Bind<Env::Configuration>()
            .ToFunc([this](DI::IServiceProvider*, Memory::RefCountedObjectBase** result) {
                std::pmr::memory_resource* allocator = Env::GetStaticAllocator(Memory::StaticAllocatorType::kLinear);
                *result = Rc<Env::Configuration>::New(allocator, m_commandLine);
                return DI::ResultCode::kSuccess;
            })
            .InSingletonScope();

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

        DI::IServiceProvider* serviceProvider = Env::GetServiceProvider();
        m_jobSystem = serviceProvider->ResolveRequired<IJobSystem>();
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
        m_frameJob.Schedule(m_jobSystem.Get(), FiberAffinityMask::kMainThread, m_exitWaitGroup.Get(), JobPriority::kHigh);
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

        Console::Flush();
        m_application->m_jobSystem->Stop();
        m_application->m_exitCode = 0;
    }


    void Application::RegisterServices([[maybe_unused]] const DI::ServiceRegistryBuilder& builder) {}
} // namespace FE::Framework

namespace FE
{
    festd::span<const festd::string_view> CommandLine::Get()
    {
        auto& app = Framework::Application::Get();
        return app.m_commandLine;
    }


    bool CommandLine::Check(const festd::string_view argument)
    {
        auto& app = Framework::Application::Get();
        const auto it = app.m_commandLineArguments.find(argument);
        return it != app.m_commandLineArguments.end();
    }


    festd::optional<festd::string_view> CommandLine::GetValue(const festd::string_view argument)
    {
        auto& app = Framework::Application::Get();
        const auto it = app.m_commandLineArguments.find(argument);
        if (it != app.m_commandLineArguments.end())
        {
            const uint32_t index = it->second + 1;
            if (index < app.m_commandLine.size())
                return app.m_commandLine[index];
        }

        return festd::nullopt;
    }
} // namespace FE
