#pragma once
#include <FeCore/Logging/Logger.h>
#include <Framework/Application/Application.h>

namespace FE::AssetBuilder
{
    struct App final : public Framework::Application
    {
        App(int32_t argc, const char** argv);

        void InitializeApp();

    private:
        Rc<WaitGroup> ScheduleUpdate() override;

        festd::unique_ptr<Framework::StdoutLogSink> m_logSink;
        Rc<Logger> m_logger;
    };
} // namespace FE::AssetBuilder
