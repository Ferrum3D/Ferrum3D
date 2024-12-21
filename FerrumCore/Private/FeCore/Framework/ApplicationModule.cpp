#include <FeCore/Assets/AssetManager.h>
#include <FeCore/Assets/IAssetLoader.h>
#include <FeCore/Console/Console.h>
#include <FeCore/EventBus/CoreEvents.h>
#include <FeCore/EventBus/EventBus.h>
#include <FeCore/Framework/ApplicationModule.h>
#include <FeCore/IO/AsyncStreamIO.h>
#include <FeCore/IO/FileStream.h>
#include <FeCore/IO/StreamFactory.h>
#include <FeCore/Modules/Configuration.h>
#include <FeCore/Modules/EnvironmentPrivate.h>

namespace FE
{
    struct LogColorScope final
    {
        explicit LogColorScope(LogSeverity severity)
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

        void Log(LogSeverity severity, SourceLocation sourceLocation, StringSlice message)
        {
            {
                const auto date = DateTime<TZ::UTC>::Now().ToString(DateTimeFormatKind::kISO8601);
                Console::Write(date);
            }

            Console::Write(" [");

            {
                LogColorScope colorScope{ severity };
                Console::Write(LogSeverityToString(severity));
            }

            Console::Write("] ");

            {
                const auto location = Fmt::FixedFormatSized<IO::kMaxPathLength + 16>(
                    "{}({}): ", sourceLocation.m_fileName, sourceLocation.m_lineNumber);
                Console::Write(StringSlice(location.Data(), location.Size()));
            }

            Console::Write(StringSlice(message.Data(), message.Size()));
            Console::Write("\n");

            if (severity >= LogSeverity::kWarning)
                Console::Flush();
        }
    };


    ApplicationModule::ApplicationModule(int32_t argc, const char** argv)
    {
        ZoneScoped;

        for (int32_t argIndex = 1; argIndex < argc; ++argIndex)
            m_commandLine.push_back(argv[argIndex]);

        std::pmr::memory_resource* pStaticLinearAllocator = Env::GetStaticAllocator(Memory::StaticAllocatorType::Linear);
        m_moduleRegistry = Rc<ModuleRegistry>::New(pStaticLinearAllocator);

        DI::ServiceRegistryBuilder builder{ Env::Internal::GetRootServiceRegistry() };
        RegisterServices(builder);
        builder.Build();

        Logger* logger = Env::GetServiceProvider()->ResolveRequired<Logger>();
        m_logSinks.push_back(festd::make_unique<StdoutLogSink>(logger));
    }


    void ApplicationModule::Initialize()
    {
        m_frameEventBus = Rc<EventBus<FrameEvents>>::DefaultNew();
    }

    int32_t ApplicationModule::RunMainLoop()
    {
        FrameEventArgs frameEventArgs{};
        frameEventArgs.m_deltaTime = 0.1f;
        frameEventArgs.m_frameIndex = 0;

        FrameMark;

        auto ts = std::chrono::high_resolution_clock::now();
        while (!ShouldStop())
        {
            ZoneScopedN("Frame");

            auto getDelta = [](std::chrono::high_resolution_clock::time_point ts) {
                auto now = std::chrono::high_resolution_clock::now();
                auto ms = std::chrono::duration_cast<std::chrono::microseconds>(now - ts).count();
                return static_cast<float>(ms) / 1'000'000;
            };

            PollSystemEvents();

            frameEventArgs.m_deltaTime = getDelta(ts);
            EventBus<FrameEvents>::SendEvent(&FrameEvents::OnFrameStart, frameEventArgs);
            frameEventArgs.m_deltaTime = getDelta(ts);
            EventBus<FrameEvents>::SendEvent(&FrameEvents::OnUpdate, frameEventArgs);

            Tick(frameEventArgs);

            frameEventArgs.m_deltaTime = getDelta(ts);
            EventBus<FrameEvents>::SendEvent(&FrameEvents::OnLateUpdate, frameEventArgs);
            frameEventArgs.m_deltaTime = getDelta(ts);
            EventBus<FrameEvents>::SendEvent(&FrameEvents::OnFrameEnd, frameEventArgs);

            frameEventArgs.m_frameIndex = ++m_frameCounter;

            ts = std::chrono::high_resolution_clock::now();

            Console::Flush();

            FrameMark;
        }

        Env::GetServiceProvider()->ResolveRequired<IJobSystem>()->Stop();

        return m_exitCode;
    }


    void ApplicationModule::RegisterServices(DI::ServiceRegistryBuilder builder)
    {
        ZoneScoped;
        builder.Bind<Env::Configuration>()
            .ToFunc([this](DI::IServiceProvider*, Memory::RefCountedObjectBase** result) {
                std::pmr::memory_resource* allocator = Env::GetStaticAllocator(Memory::StaticAllocatorType::Linear);
                *result = Memory::New<Env::Configuration>(allocator, m_commandLine);
                return DI::ResultCode::kSuccess;
            })
            .InSingletonScope();

        builder.Bind<IJobSystem>().To<JobSystem>().InSingletonScope();
        builder.Bind<Logger>().ToSelf().InSingletonScope();
        builder.Bind<Assets::IAssetManager>().To<Assets::AssetManager>().InSingletonScope();
        builder.Bind<IO::IStreamFactory>().To<IO::FileStreamFactory>().InSingletonScope();
        builder.Bind<IO::IAsyncStreamIO>().To<IO::AsyncStreamIO>().InSingletonScope();
    }


    bool ApplicationModule::ShouldStop()
    {
        return CloseEventReceived() || m_stopRequested;
    }

    void ApplicationModule::Stop(int32_t exitCode)
    {
        m_exitCode = exitCode;
        m_stopRequested = true;
    }
} // namespace FE
