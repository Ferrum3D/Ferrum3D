#pragma once
#include <Core/CLI/CommandLine.h>

namespace FE::FerrumCli
{
    struct CommandLineParser;

    struct FE_META(Cli::Parent = FE::FerrumCli::CommandLineParser; Cli::Description = Build an asset) Build final
        : public Cli::Subcommand
    {
        FE_RTTI("4D105B8D-0E84-460B-98BC-8A4C3710A193");
        FE_RTTI_Reflect();

        Cli::Flag m_help FE_META(Cli::Description = Print this help message);
        Cli::Option m_asset FE_META(Cli::Description = Asset to build; Cli::ValueName = path);
    };

    struct CommandLineParser final : public Cli::Parser
    {
        FE_RTTI("2B409CCD-3CA4-46A0-BA21-48B1D92AED2B");
        FE_RTTI_Reflect();

        Cli::Flag m_help FE_META(Cli::Description = Print this help message);
        Cli::Flag m_version FE_META(Cli::Description = Print version);
    };
} // namespace FE::FerrumCli
