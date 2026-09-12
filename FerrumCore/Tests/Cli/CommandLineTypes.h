#pragma once
#include <Core/CLI/CommandLine.h>

namespace FE::Cli::Tests
{
    struct TestParser;

    struct FE_META(Rtti::DisplayName("Build") + Cli::Parent("FE::Cli::Tests::TestParser") + Cli::Description("Build an asset"))
        Build final : public Subcommand
    {
        FE_RTTI("D5DD1441-E7EC-48E5-9706-15434AA507D5");
        FE_RTTI_Reflect();

        FE_META(Cli::Description("Print this help message"))
        Flag m_help;
        FE_META(Cli::Description("Asset to build") + Cli::ValueName("path") + Rtti::IntRange(2, 3))
        Option m_asset;
    };

    struct FE_META(Cli::Description("Test command line")) TestParser final : public Parser
    {
        FE_RTTI("EF03AE87-B2DF-4FA0-A77D-B92CC8E2015C");
        FE_RTTI_Reflect();

        FE_META(Cli::Description("Print this help message"))
        Flag m_help;
        FE_META(Cli::Description("Print version"))
        Flag m_version;
    };
} // namespace FE::Cli::Tests
