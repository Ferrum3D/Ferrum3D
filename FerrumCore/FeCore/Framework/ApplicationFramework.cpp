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

        m_Logger = Rc<Debug::ConsoleLogger>::DefaultNew();
        m_Logger->SetDebugLevel(desc.LogSeverity);
        m_FrameEventBus = Rc<EventBus<FrameEvents>>::DefaultNew();
        m_JobSystem = Rc<JobSystem>::DefaultNew();

        m_AssetManager = Rc<Assets::AssetManager>::DefaultNew();
        Rc assetProvider = Rc<Assets::AssetProviderDev>::DefaultNew();
        if (!Desc.AssetDirectory.Empty())
        {
            Rc assetRegistry = Rc<Assets::AssetRegistry>::DefaultNew();
            assetRegistry->LoadAssetsFromFile(Desc.AssetDirectory / "FerrumAssetIndex");
            assetProvider->AttachRegistry(assetRegistry);
        }
        m_AssetManager->AttachAssetProvider(assetProvider);

        FrameworkBase::Initialize();
    }

    int32_t ApplicationFramework::RunMainLoop()
    {
        FrameEventArgs frameEventArgs{};
        frameEventArgs.DeltaTime = 0.1f;
        frameEventArgs.FrameIndex = 0;

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
        }

        return m_ExitCode;
    }

    bool ApplicationFramework::ShouldStop()
    {
        return CloseEventReceived() || m_StopRequested;
    }

    void ApplicationFramework::Stop(int32_t exitCode)
    {
        m_ExitCode = exitCode;
        m_StopRequested = true;
    }

    ApplicationFramework::~ApplicationFramework()
    {
        FrameworkBase::UnloadDependencies();
    }
} // namespace FE
