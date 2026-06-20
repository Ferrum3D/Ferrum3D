#pragma once
#include <Core/Logging/Logger.h>
#include <FerrumCli/CommandLine.h>
#include <Framework/Application/Application.h>

namespace FE::FerrumCli
{
    struct App final
    {
        explicit App(festd::span<const festd::string_view> commandLine);

        int32_t Run();

    private:
        void PrintHelp(const Cli::Command& command);

        CommandLineParser m_cli;
        festd::unique_ptr<Framework::StdoutLogSink> m_logSink;
        Rc<Logger> m_logger;
    };
} // namespace FE::FerrumCli
