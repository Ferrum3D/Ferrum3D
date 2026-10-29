#pragma once
#include <Core/CLI/CommandLine.h>

namespace FE::FerrumCli
{
    struct CommandLineParser;

    struct FE_META(Cli::Parent("FE::FerrumCli::CommandLineParser") + Cli::Description("Import source assets")) Import final
        : public Cli::Subcommand
    {
        FE_RTTI("3F4138AA-84A9-4C02-889B-0E2B090BE723");
        FE_RTTI_Reflect();

        FE_META(Cli::Description("Print this help message"))
        Cli::Flag m_help;
        FE_META(Cli::Description("Source asset to import") + Cli::ValueName("path"))
        Cli::Option m_asset;
        FE_META(Cli::Description("Type UUID for a source-less serialized asset") + Cli::ValueName("UUID"))
        Cli::Option m_assetType;
        FE_META(Cli::Description("Generated .asset file") + Cli::ValueName("path"))
        Cli::Option m_output;
    };


    struct FE_META(Cli::Parent("FE::FerrumCli::CommandLineParser") + Cli::Description("Build an asset graph")) Build final
        : public Cli::Subcommand
    {
        FE_RTTI("4D105B8D-0E84-460B-98BC-8A4C3710A193");
        FE_RTTI_Reflect();

        FE_META(Cli::Description("Print this help message"))
        Cli::Flag m_help;
        FE_META(Cli::Description(".asset file to build") + Cli::ValueName("path"))
        Cli::Option m_asset;
        FE_META(Cli::Description("Build every .asset file in the depots, in dependency order"))
        Cli::Flag m_all;
        FE_META(Cli::Description("Depot directories searched for .asset files, separated by ';'") + Cli::ValueName("directories"))
        Cli::Option m_depot;
        FE_META(Cli::Description("Asset source depot root (required for file-backed assets)") + Cli::ValueName("directory"))
        Cli::Option m_sourceRoot;
        FE_META(Cli::Description("Artifact output directory") + Cli::ValueName("directory"))
        Cli::Option m_output;
    };

    struct FE_META(Cli::Parent("FE::FerrumCli::CommandLineParser")
                   + Cli::Description("Keep the artifacts of depots up to date while running")) Serve final
        : public Cli::Subcommand
    {
        FE_RTTI("F282375C-2C98-4CFF-B467-D05FD38283DD");
        FE_RTTI_Reflect();

        FE_META(Cli::Description("Print this help message"))
        Cli::Flag m_help;
        FE_META(Cli::Description("Depot directories searched for .asset files, separated by ';'") + Cli::ValueName("directories"))
        Cli::Option m_depot;
        FE_META(Cli::Description("Asset source depot root, for a single depot") + Cli::ValueName("directory"))
        Cli::Option m_sourceRoot;
        FE_META(Cli::Description("Artifact output directory") + Cli::ValueName("directory"))
        Cli::Option m_output;
        FE_META(Cli::Description("Check every asset at startup instead of only the files changed since the previous run"))
        Cli::Flag m_fullScan;
        FE_META(Cli::Description("Watch directories instead of reading the volume change journal"))
        Cli::Flag m_noJournal;
    };


    struct FE_META(Cli::Parent("FE::FerrumCli::CommandLineParser")
                   + Cli::Description("Print the status of a running asset server")) Status final : public Cli::Subcommand
    {
        FE_RTTI("CDEDACC0-8921-43BC-AB33-78F82828CCE5");
        FE_RTTI_Reflect();

        FE_META(Cli::Description("Print this help message"))
        Cli::Flag m_help;
        FE_META(Cli::Description("Artifact output directory of the server") + Cli::ValueName("directory"))
        Cli::Option m_output;
    };


    struct FE_META(Cli::Parent("FE::FerrumCli::CommandLineParser")
                   + Cli::Description("Compile the shader permutation manifest of a catalog into its precompiled shaders"))
        BuildShaderCache final : public Cli::Subcommand
    {
        FE_RTTI("B3EAB519-1B54-458B-8D47-74169A925F52");
        FE_RTTI_Reflect();

        FE_META(Cli::Description("Print this help message"))
        Cli::Flag m_help;
        FE_META(Cli::Description("Depot directories with the shader packages, separated by ';'") + Cli::ValueName("directories"))
        Cli::Option m_depot;
        FE_META(Cli::Description("Asset source depot root, for a single depot") + Cli::ValueName("directory"))
        Cli::Option m_sourceRoot;
        FE_META(Cli::Description("Artifact output directory with the manifest") + Cli::ValueName("directory"))
        Cli::Option m_output;
        FE_META(Cli::Description("Compile unoptimized shaders with debug information instead of optimized ones"))
        Cli::Flag m_debug;
    };


    struct CommandLineParser final : public Cli::Parser
    {
        FE_RTTI("2B409CCD-3CA4-46A0-BA21-48B1D92AED2B");
        FE_RTTI_Reflect();

        FE_META(Cli::Description("Print this help message"))
        Cli::Flag m_help;
        FE_META(Cli::Description("Print version"))
        Cli::Flag m_version;
    };
} // namespace FE::FerrumCli
