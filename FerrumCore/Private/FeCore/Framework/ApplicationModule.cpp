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
        inline LogColorScope(LogSeverity severity)
        {
            Console::Color color = Console::Color::Default;
            switch (severity)
            {
            case LogSeverity::kWarning:
                color = Console::Color::Yellow;
                break;
            case LogSeverity::kError:
            case LogSeverity::kCritical:
                color = Console::Color::Red;
                break;
            }

            Console::SetColor(color);
        }

        inline ~LogColorScope()
        {
            Console::ResetColor();
        }
    };


    struct StdoutLogSink final : public LogSinkBase
    {
        Rc<IO::FileStream> m_stream;

        inline StdoutLogSink(Logger* logger)
            : LogSinkBase(logger)
        {
            m_stream = Rc<IO::FileStream>::DefaultNew();
            m_stream->Open(IO::StandardDescriptor::kSTDOUT);
            m_stream->EnsureBufferAllocated();
        }

        inline void Log(LogSeverity severity, SourceLocation sourceLocation, StringSlice message)
        {
            {
                const auto date = DateTime<TZ::UTC>::Now().ToString(DateTimeFormatKind::kISO8601);
                m_stream->WriteFromBuffer(Memory::MakeByteSpan(date.Data(), date.Size()));
            }

            m_stream->WriteFromBuffer(Memory::MakeByteSpan(" ["));

            {
                LogColorScope colorScope{ severity };
                m_stream->WriteFromBuffer(Memory::MakeByteSpan(LogSeverityToString(severity)));
            }

            m_stream->WriteFromBuffer(Memory::MakeByteSpan("] "));

            {
                const auto location =
                    Fmt::FixedFormatSized<IO::MaxPathLength + 16>("{}({}): ", sourceLocation.FileName, sourceLocation.LineNumber);
                m_stream->WriteFromBuffer(Memory::MakeByteSpan(location.Data(), location.Size()));
            }

            m_stream->WriteFromBuffer(Memory::MakeByteSpan(message.Data(), message.Size()));
            m_stream->WriteFromBuffer(Memory::MakeByteSpan("\n"));

            if (severity >= LogSeverity::kWarning)
                m_stream->FlushWrites();
        }
    };


    ApplicationModule::ApplicationModule(int32_t argc, const char** argv)
    {
        ZoneScoped;

        for (int32_t argIndex = 1; argIndex < argc; ++argIndex)
            m_commandLine.push_back(argv[argIndex]);

        std::pmr::memory_resource* pStaticLinearAllocator = Env::GetStaticAllocator(Memory::StaticAllocatorType::Linear);
        m_ModuleRegistry = Rc<ModuleRegistry>::New(pStaticLinearAllocator);

        DI::ServiceRegistryBuilder builder{ Env::Internal::GetRootServiceRegistry() };
        RegisterServices(builder);
        builder.Build();

        Logger* logger = Env::GetServiceProvider()->ResolveRequired<Logger>();
        m_logSinks.push_back(festd::make_unique<StdoutLogSink>(logger));
    }


    void ApplicationModule::Initialize()
    {
        m_FrameEventBus = Rc<EventBus<FrameEvents>>::DefaultNew();
    }

    int32_t ApplicationModule::RunMainLoop()
    {
        FrameEventArgs frameEventArgs{};
        frameEventArgs.DeltaTime = 0.1f;
        frameEventArgs.FrameIndex = 0;

        FrameMark;

        auto ts = std::chrono::high_resolution_clock::now();
        while (!ShouldStop())
        {
            auto getDelta = [](std::chrono::high_resolution_clock::time_point ts) {
                auto now = std::chrono::high_resolution_clock::now();
                auto ms = std::chrono::duration_cast<std::chrono::microseconds>(now - ts).count();
                return static_cast<float>(ms) / 1'000'000;
            };

            PollSystemEvents();

            frameEventArgs.DeltaTime = getDelta(ts);
            EventBus<FrameEvents>::SendEvent(&FrameEvents::OnFrameStart, frameEventArgs);
            frameEventArgs.DeltaTime = getDelta(ts);
            EventBus<FrameEvents>::SendEvent(&FrameEvents::OnUpdate, frameEventArgs);

            Tick(frameEventArgs);

            frameEventArgs.DeltaTime = getDelta(ts);
            EventBus<FrameEvents>::SendEvent(&FrameEvents::OnLateUpdate, frameEventArgs);
            frameEventArgs.DeltaTime = getDelta(ts);
            EventBus<FrameEvents>::SendEvent(&FrameEvents::OnFrameEnd, frameEventArgs);

            frameEventArgs.FrameIndex = ++m_FrameCounter;

            ts = std::chrono::high_resolution_clock::now();

            FrameMark;
        }

        Env::GetServiceProvider()->ResolveRequired<IJobSystem>()->Stop();

        return m_ExitCode;
    }


    void ApplicationModule::RegisterServices(DI::ServiceRegistryBuilder builder)
    {
        ZoneScoped;
        builder.Bind<Env::Configuration>()
            .ToFunc([this](DI::IServiceProvider*, Memory::RefCountedObjectBase** result) {
                std::pmr::memory_resource* allocator = Env::GetStaticAllocator(Memory::StaticAllocatorType::Linear);
                *result = Memory::New<Env::Configuration>(allocator, m_commandLine);
                return DI::ResultCode::Success;
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
        return CloseEventReceived() || m_StopRequested;
    }

    void ApplicationModule::Stop(int32_t exitCode)
    {
        m_ExitCode = exitCode;
        m_StopRequested = true;
    }
} // namespace FE
