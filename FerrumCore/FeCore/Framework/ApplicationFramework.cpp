#include <FeCore/Assets/AssetManager.h>
#include <FeCore/Assets/AssetProviderDev.h>
#include <FeCore/Assets/IAssetLoader.h>
#include <FeCore/EventBus/EventBus.h>
#include <FeCore/EventBus/FrameEvents.h>
#include <FeCore/Framework/ApplicationFramework.h>

namespace FE
{
    void ApplicationFramework::Initialize(const ApplicationDesc& desc)
    {
        Desc = desc;

        m_Logger        = MakeShared<Debug::ConsoleLogger>();
        m_FrameEventBus = MakeShared<EventBus<FrameEvents>>();
        m_JobScheduler  = MakeShared<JobScheduler>(std::thread::hardware_concurrency() - 1);

        if (!Desc.AssetDirectory.Empty())
        {
            m_AssetManager     = MakeShared<Assets::AssetManager>();
            auto assetProvider = MakeShared<Assets::AssetProviderDev>();
            auto assetRegistry = FE::MakeShared<FE::Assets::AssetRegistry>();
            assetRegistry->LoadAssetsFromFile(Desc.AssetDirectory / "FerrumAssetIndex");
            assetProvider->AttachRegistry(assetRegistry);
            m_AssetManager->AttachAssetProvider(assetProvider);
        }

        FrameworkBase::Initialize();
    }

    Int32 ApplicationFramework::RunMainLoop()
    {
        FrameEventArgs frameEventArgs{};
        frameEventArgs.DeltaTime  = 0.1f;
        frameEventArgs.FrameIndex = 0;

        auto ts = std::chrono::high_resolution_clock::now();
        while (!ShouldStop())
        {
            auto getDelta = [](std::chrono::high_resolution_clock::time_point ts) {
                auto now = std::chrono::high_resolution_clock::now();
                auto ms  = std::chrono::duration_cast<std::chrono::microseconds>(now - ts).count();
                return static_cast<FE::Float32>(ms) / 1'000'000;
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
        }

        return m_ExitCode;
    }

    bool ApplicationFramework::ShouldStop()
    {
        return CloseEventReceived() || m_StopRequested;
    }

    void ApplicationFramework::Stop(Int32 exitCode)
    {
        m_ExitCode      = exitCode;
        m_StopRequested = true;
    }

    ApplicationFramework::~ApplicationFramework()
    {
        FrameworkBase::UnloadDependencies();
    }
} // namespace FE
