#pragma once
#include <FeCore/DI/BaseDI.h>
#include <FeCore/Jobs/Job.h>
#include <Framework/Application/Core/PlatformApplication.h>
#include <Framework/Application/Core/PlatformWindow.h>
#include <festd/unordered_map.h>
#include <festd/vector.h>

namespace FE
{
    struct CommandLine final
    {
        static festd::span<const festd::string_view> Get();
        static bool Check(festd::string_view argument);
        static festd::optional<festd::string_view> GetValue(festd::string_view argument);
    };
} // namespace FE


namespace FE::Framework
{
    struct StdoutLogSink final : public LogSinkBase
    {
        StdoutLogSink(Logger* logger)
            : LogSinkBase(logger)
        {
        }

        void Log(LogSeverity severity, SourceLocation sourceLocation, festd::string_view message) override;
    };


    struct Application : public Memory::RefCountedObjectBase
    {
        FE_RTTI_Class(Application, "AF07EDCA-2D55-4E2D-A5EF-85ED53B4CDAB");

        Application(int32_t argc, const char** argv);
        ~Application() override;

        Application(const Application&) = delete;
        Application(Application&&) = delete;
        Application& operator=(const Application&) = delete;
        Application& operator=(Application&&) = delete;

        void InitializeCore();
        void InitializeWindow();

        int32_t Run();

        static Application& Get();

    protected:
        friend CommandLine;

        struct FrameJob final : public Job
        {
            void Execute() override;

            Application* m_application = nullptr;
        };

        virtual void RegisterServices(const DI::ServiceRegistryBuilder& builder);

        virtual Rc<WaitGroup> ScheduleUpdate() = 0;

        festd::vector<festd::string_view> m_commandLine;
        festd::unordered_dense_map<festd::string_view, uint32_t> m_commandLineArguments;

        Rc<Core::PlatformApplication> m_platformApplication;
        Rc<Core::PlatformWindow> m_mainWindow;
        Rc<IJobSystem> m_jobSystem;
        Rc<WaitGroup> m_exitWaitGroup;
        FrameJob m_frameJob;
        int32_t m_exitCode = 0;
    };
} // namespace FE::Framework
