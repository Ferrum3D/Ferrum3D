﻿#pragma once
#include <FeCore/Assets/IAssetManager.h>
#include <FeCore/EventBus/CoreEvents.h>
#include <FeCore/EventBus/EventBus.h>
#include <FeCore/Framework/ModuleBase.h>
#include <FeCore/Jobs/JobSystem.h>

namespace FE
{
    //! @brief Base class for all applications created with Ferrum3D.
    //!
    //! Usage:
    //! \code{.cpp}
    //!     class MyApplication : public ApplicationModule { ... };
    //!
    //!     // To create and run:
    //!     int main(int argc, char** argv)
    //!     {
    //!         return ApplicationModule::Run(argc, argv, [](MyApplication* app){ app->Initialize(); });
    //!     }
    //!
    //!     // To refer to the app instance (e.g. to exit):
    //!     ServiceLocator<ApplicationModule>::Get().Stop(errorCode);
    //! \endcode
    class ApplicationModule : public ServiceLocatorImplBase<IModule, ApplicationModule>
    {
        uint32_t m_FrameCounter = 0;
        int32_t m_ExitCode = 0;
        bool m_StopRequested = false;

        Rc<ModuleRegistry> m_ModuleRegistry;
        Rc<EventBus<FrameEvents>> m_FrameEventBus;

        festd::vector<festd::unique_ptr<LogSinkBase>> m_logSinks;

        festd::vector<StringSlice> m_commandLine;

        template<class TApplication, class TFunc>
        struct MainJob final : public Job
        {
            TFunc m_initFunc;
            TApplication* m_app = nullptr;
            int32_t m_resultCode = -1;

            MainJob(TFunc&& initFunc, TApplication* app)
                : m_initFunc(std::forward<TFunc>(initFunc))
                , m_app(app)
            {
            }

            void Execute() override
            {
                m_initFunc(m_app);
                m_resultCode = m_app->RunMainLoop();
            }
        };

    protected:
        virtual void PollSystemEvents() = 0;
        virtual bool CloseEventReceived() = 0;

        inline virtual void Tick([[maybe_unused]] const FrameEventArgs& frameEventArgs) {}
        void RegisterServices(DI::ServiceRegistryBuilder builder) override;

        virtual bool ShouldStop();

    public:
        FE_RTTI_Class(ApplicationModule, "CF197AF1-C4AE-4048-A85E-CEF4F03490AD");

        ApplicationModule(int32_t argc, const char** argv);
        ~ApplicationModule() override = default;

        virtual void Initialize();
        int32_t RunMainLoop();
        void Stop(int32_t exitCode);

        template<class TApplication, class TFunc, class = std::enable_if_t<std::is_base_of_v<ApplicationModule, TApplication>>>
        inline static int32_t Run(int32_t argc, const char** argv, TFunc&& initFunc)
        {
            Env::CreateEnvironment();

            std::pmr::memory_resource* allocator = Env::GetStaticAllocator(Memory::StaticAllocatorType::Linear);

            TApplication* application = Memory::New<TApplication>(allocator, argc, argv);

            IJobSystem* jobSystem = Env::GetServiceProvider()->ResolveRequired<IJobSystem>();

            MainJob<TApplication, TFunc> mainJob{ std::forward<TFunc>(initFunc), application };
            mainJob.Schedule(jobSystem);
            jobSystem->Start();

            application->~TApplication();
            return mainJob.m_resultCode;
        }
    };
} // namespace FE