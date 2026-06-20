#pragma once
#include <Core/Cli/CommandLine.h>

namespace FE::FerrumCli
{
    struct CommandLineParser;

    struct FE_ATTRIBUTE(Cli::Parent = FE::FerrumCli::CommandLineParser, Cli::Description = Build an asset) Build final
        : public Cli::Subcommand
    {
        FE_RTTI_Reflect("4D105B8D-0E84-460B-98BC-8A4C3710A193");

        Cli::Flag m_help FE_ATTRIBUTE(Cli::Description = Print this help message);
        Cli::Option m_asset FE_ATTRIBUTE(Cli::Description = Asset to build, Cli::ValueName = path);
    };

    struct CommandLineParser final : public Cli::Parser
    {
        FE_RTTI_Reflect("2B409CCD-3CA4-46A0-BA21-48B1D92AED2B");

        Cli::Flag m_help FE_ATTRIBUTE(Cli::Description = Print this help message);
        Cli::Flag m_version FE_ATTRIBUTE(Cli::Description = Print version);
    };
} // namespace FE::FerrumCli
