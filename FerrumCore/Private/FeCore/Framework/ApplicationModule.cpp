#include <FeCore/Assets/AssetManager.h>
#include <FeCore/Assets/IAssetLoader.h>
#include <FeCore/Console/Console.h>
#include <FeCore/EventBus/CoreEvents.h>
#include <FeCore/EventBus/EventBus.h>
#include <FeCore/Framework/ApplicationModule.h>
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
        Rc<IO::FileStream> Stream;

        inline StdoutLogSink(Logger* logger)
            : LogSinkBase(logger)
        {
            Stream = Rc<IO::FileStream>::DefaultNew();
            Stream->Open(IO::StandardDescriptor::kSTDOUT);
            Stream->EnsureBufferAllocated();
        }

        inline void Log(LogSeverity severity, SourceLocation sourceLocation, StringSlice message)
        {
            {
                const auto date = DateTime<TZ::UTC>::Now().ToString(DateTimeFormatKind::kISO8601);
                Stream->WriteFromBuffer(Memory::MakeByteSpan(date.Data(), date.Size()));
            }

            Stream->WriteFromBuffer(Memory::MakeByteSpan(" ["));

            {
                LogColorScope colorScope{ severity };
                Stream->WriteFromBuffer(Memory::MakeByteSpan(LogSeverityToString(severity)));
            }

            Stream->WriteFromBuffer(Memory::MakeByteSpan("] "));

            {
                const auto location =
                    Fmt::FixedFormatSized<IO::MaxPathLength + 16>("{}({}): ", sourceLocation.FileName, sourceLocation.LineNumber);
                Stream->WriteFromBuffer(Memory::MakeByteSpan(location.Data(), location.Size()));
            }

            Stream->WriteFromBuffer(Memory::MakeByteSpan(message.Data(), message.Size()));
            Stream->WriteFromBuffer(Memory::MakeByteSpan("\n"));

            if (severity >= LogSeverity::kWarning)
                Stream->FlushWrites();
        }
    };


    ApplicationModule::ApplicationModule()
    {
        ZoneScoped;
        std::pmr::memory_resource* pStaticLinearAllocator = Env::GetStaticAllocator(Memory::StaticAllocatorType::Linear);
        m_ModuleRegistry = Rc<ModuleRegistry>::New(pStaticLinearAllocator);

        DI::ServiceRegistryBuilder builder{ Env::Internal::GetRootServiceRegistry() };
        RegisterServices(builder);
        builder.Build();
    }


    void ApplicationModule::Initialize()
    {
        ZoneScoped;
        m_FrameEventBus = Rc<EventBus<FrameEvents>>::DefaultNew();

        Rc assetProvider = Rc<Assets::AssetProviderDev>::DefaultNew();
        if (!m_AssetDirectory.Empty())
        {
            Rc assetRegistry = Rc<Assets::AssetRegistry>::DefaultNew();
            assetRegistry->LoadAssetsFromFile(m_AssetDirectory / "FerrumAssetIndex");
            assetProvider->AttachRegistry(assetRegistry);
        }

        DI::IServiceProvider* pServiceProvider = Env::GetServiceProvider();
        Assets::IAssetManager* pAssetManager = pServiceProvider->ResolveRequired<Assets::IAssetManager>();
        pAssetManager->AttachAssetProvider(assetProvider);
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

        return m_ExitCode;
    }


    void ApplicationModule::RegisterServices(DI::ServiceRegistryBuilder builder)
    {
        ZoneScoped;
        builder.Bind<IJobSystem>().To<JobSystem>().InSingletonScope();
        builder.Bind<Env::Configuration>().ToSelf().InSingletonScope();
        builder.Bind<Logger>().ToSelf().InSingletonScope();
        builder.Bind<Assets::IAssetManager>().To<Assets::AssetManager>().InSingletonScope();
        builder.Bind<IO::IStreamFactory>().To<IO::FileStreamFactory>().InSingletonScope();
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
