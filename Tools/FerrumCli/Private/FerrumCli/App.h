#pragma once
#include <FerrumCli/CommandLine.h>

namespace FE::FerrumCli
{
    struct App final
    {
        explicit App();

        int32_t Run();

    private:
        void PrintHelp(const Cli::Command& command);
        int32_t RunImport(const Import& command);
        int32_t RunBuild(const Build& command);

        CommandLineParser m_cli;
    };
} // namespace FE::FerrumCli
