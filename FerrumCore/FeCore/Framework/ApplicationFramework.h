#pragma once
#include <FeCore/Assets/IAssetManager.h>
#include <FeCore/Console/ConsoleLogger.h>
#include <FeCore/EventBus/EventBus.h>
#include <FeCore/Framework/FrameworkBase.h>
#include <FeCore/Jobs/JobScheduler.h>

namespace FE
{
    struct ApplicationDesc
    {
        String Name;
        String AssetDirectory;
        UInt32 WindowWidth                = 800;
        UInt32 WindowHeight               = 600;
        Debug::LogMessageType LogSeverity = Debug::LogMessageType::All;
        bool Fullscreen                   = false;

        inline ApplicationDesc() = default;

        inline ApplicationDesc(const String& name, UInt32 windowWidth = 800, UInt32 windowHeight = 600, bool fullscreen = false)
            : Name(name)
            , AssetDirectory()
            , WindowWidth(windowWidth)
            , WindowHeight(windowHeight)
            , Fullscreen(fullscreen)
        {
        }

        inline ApplicationDesc(const String& name, Debug::LogMessageType logSeverity)
            : Name(name)
            , LogSeverity(logSeverity)
        {
        }

        inline ApplicationDesc(const String& name, const String& assetDirectory, UInt32 windowWidth = 800,
                               UInt32 windowHeight = 600, bool fullscreen = false)
            : Name(name)
            , AssetDirectory(assetDirectory)
            , WindowWidth(windowWidth)
            , WindowHeight(windowHeight)
            , Fullscreen(fullscreen)
        {
        }
    };

    //! \brief Base class for all applications created with Ferrum3D.
    //!
    //! Usage:
    //! \code{.cpp}
    //!     class MyApplication : public ApplicationFramework { ... };
    //!
    //!     // To create and run:
    //!     int main()
    //!     {
    //!         auto myApp = MakeShared<MyApplication>();
    //!         myApp->Initialize(ApplicationDesc{ ... });
    //!         return myApp->RunMainLoop();
    //!     }
    //!
    //!     // To refer to the app instance (e.g. to exit):
    //!     ServiceLocator<ApplicationFramework>::Get().Stop(errorCode);
    //! \endcode
    class ApplicationFramework : public ServiceLocatorImplBase<FrameworkBase, ApplicationFramework>
    {
        UInt32 m_FrameCounter = 0;
        Int32 m_ExitCode      = 0;
        bool m_StopRequested  = false;

        Rc<Debug::ConsoleLogger> m_Logger;
        Rc<EventBus<FrameEvents>> m_FrameEventBus;
        Rc<JobScheduler> m_JobScheduler;
        Rc<Assets::IAssetManager> m_AssetManager;

    protected:
        virtual void PollSystemEvents()   = 0;
        virtual bool CloseEventReceived() = 0;

        inline virtual void Tick([[maybe_unused]] const FrameEventArgs& frameEventArgs) {}

        virtual bool ShouldStop();

    public:
        ApplicationDesc Desc;

        FE_CLASS_RTTI(ApplicationFramework, "CF197AF1-C4AE-4048-A85E-CEF4F03490AD");

        ~ApplicationFramework() override;

        virtual void Initialize(const ApplicationDesc& desc);
        Int32 RunMainLoop();
        void Stop(Int32 exitCode);
    };

#define FE_APP_MAIN()                                                                                                            \
    int MainImpl(int argc, char** argv);                                                                                         \
    int main(int argc, char** argv)                                                                                              \
    {                                                                                                                            \
        ::FE::Env::CreateEnvironment();                                                                                          \
        int code = MainImpl(argc, argv);                                                                                         \
        return code;                                                                                                             \
    }                                                                                                                            \
    int MainImpl([[maybe_unused]] int argc, [[maybe_unused]] char** argv)
} // namespace FE
