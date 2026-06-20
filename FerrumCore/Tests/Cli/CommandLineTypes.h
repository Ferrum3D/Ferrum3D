#pragma once
#include <Core/Cli/CommandLine.h>

namespace FE::Cli::Tests
{
    struct TestParser;

    struct FE_ATTRIBUTE(Cli::Parent = FE::Cli::Tests::TestParser) FE_ATTRIBUTE(Cli::Description = Build an asset) Build final
        : public Subcommand
    {
        FE_RTTI_Reflect("D5DD1441-E7EC-48E5-9706-15434AA507D5");

        Flag m_help FE_ATTRIBUTE(Cli::Description = Print this help message);
        Option m_asset FE_ATTRIBUTE(Cli::Description = Asset to build) FE_ATTRIBUTE(Cli::ValueName = path);
    };

    struct FE_ATTRIBUTE(Cli::Description = Test command line) TestParser final : public Parser
    {
        FE_RTTI_Reflect("EF03AE87-B2DF-4FA0-A77D-B92CC8E2015C");

        Flag m_help FE_ATTRIBUTE(Cli::Description = Print this help message);
        Flag m_version FE_ATTRIBUTE(Cli::Description = Print version);
    };
} // namespace FE::Cli::Tests
