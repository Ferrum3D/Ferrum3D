#pragma once
#include <Core/Logging/Logger.h>
#include <FerrumCli/CommandLine.h>
#include <Framework/Application/Application.h>

namespace FE::FerrumCli
{
    struct App final
    {
        explicit App();

        int32_t Run();

    private:
        void PrintHelp(const Cli::Command& command);

        CommandLineParser m_cli;
    };
} // namespace FE::FerrumCli
