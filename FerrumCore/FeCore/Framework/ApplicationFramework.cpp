#include <FeCore/EventBus/EventBus.h>
#include <FeCore/EventBus/FrameEvents.h>
#include <FeCore/Framework/ApplicationFramework.h>

namespace FE
{
    void ApplicationFramework::Initialize(const ApplicationDesc& desc)
    {
        Desc = desc;

        m_Logger        = FE::MakeShared<FE::Debug::ConsoleLogger>();
        m_FrameEventBus = FE::MakeShared<FE::EventBus<FE::FrameEvents>>();
        m_JobScheduler  = FE::MakeShared<FE::JobScheduler>(std::thread::hardware_concurrency() - 1);
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
