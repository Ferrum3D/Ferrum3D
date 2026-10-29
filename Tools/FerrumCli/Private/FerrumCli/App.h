#pragma once
#include <FerrumCli/CommandLine.h>

namespace FE::AssetBuilder
{
    struct AssetDepot;
} // namespace FE::AssetBuilder

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
        int32_t RunBuildAll(const Build& command);
        int32_t RunServe(const Serve& command);
        int32_t RunStatus(const Status& command);
        int32_t RunBuildShaderCache(const BuildShaderCache& command);
        bool ScanDepot(const Cli::Command& command, const Cli::Option& depot, const Cli::Option& sourceRoot,
                       const Cli::Option& output, AssetBuilder::AssetDepot& result);

        CommandLineParser m_cli;
    };
} // namespace FE::FerrumCli
