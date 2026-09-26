#include <FerrumCli/App.h>

#include <AssetBuilder/AssetPipeline.h>
#include <Core/IO/BaseIO.h>

namespace FE::FerrumCli
{
    App::App()
    {
        std::pmr::memory_resource* allocator = Env::GetStaticAllocator(Memory::StaticAllocatorType::kLinear);
        m_cli = Cli::Parse<CommandLineParser>(allocator, Cli::GetArgs());
    }


    void App::PrintHelp(const Cli::Command& command)
    {
        std::pmr::memory_resource* allocator = Env::GetStaticAllocator(Memory::StaticAllocatorType::kLinear);
        const festd::pmr::string help = Cli::BuildHelp(allocator, "ferrum", command);
        IO::Print("{}", help);
    }


    int32_t App::Run()
    {
        if (!m_cli.IsValid())
        {
            IO::EPrintLn("Error: {}", m_cli.GetError());
            PrintHelp(m_cli);
            return 1;
        }

        if (m_cli.m_help)
        {
            PrintHelp(m_cli);
            return 0;
        }

        if (m_cli.m_version)
        {
            IO::PrintLn("Ferrum {}", FE_FERRUM_VERSION);
            return 0;
        }

        if (const Import* import = m_cli.GetSubcommand<Import>())
        {
            if (import->m_help)
            {
                PrintHelp(*import);
                return 0;
            }

            const bool hasSourceAsset = import->m_asset && !import->m_asset.Get().empty();
            const bool hasAssetType = import->m_assetType && !import->m_assetType.Get().empty();
            if (hasSourceAsset == hasAssetType)
            {
                IO::EPrintLn("Error: Specify exactly one of '--asset' or '--asset-type'");
                PrintHelp(*import);
                return 1;
            }

            IO::Path outputPath;
            if (import->m_output)
            {
                outputPath = import->m_output.Get();
            }
            else if (hasSourceAsset)
            {
                const IO::PathView sourcePath(import->m_asset.Get());
                outputPath = sourcePath.parent_directory();
                IO::Path outputName(sourcePath.stem());
                outputName.AsBaseString() += ".asset";
                outputPath /= outputName;
            }
            else
            {
                IO::EPrintLn("Error: Option '--output' is required with '--asset-type'");
                PrintHelp(*import);
                return 1;
            }

            AssetBuilder::ImportAssetSettings settings;
            if (hasSourceAsset)
            {
                settings.m_inputFile = import->m_asset.Get();
            }
            else
            {
                const festd::string_view assetType = import->m_assetType.Get();
                settings.m_assetTypeId = Rtti::TypeID(festd::ascii_view{ assetType.data(), assetType.size() });
            }

            settings.m_outputFile = outputPath;
            return AssetBuilder::ImportAsset(settings) ? 0 : 1;
        }

        if (const Build* build = m_cli.GetSubcommand<Build>())
        {
            if (build->m_help)
            {
                PrintHelp(*build);
                return 0;
            }

            if (!build->m_asset || build->m_asset.Get().empty())
            {
                IO::EPrintLn("Error: Option '--asset' is required");
                PrintHelp(*build);
                return 1;
            }

            if (!build->m_output || build->m_output.Get().empty())
            {
                IO::EPrintLn("Error: Option '--output' is required");
                PrintHelp(*build);
                return 1;
            }

            const IO::PathView assetFilePath(build->m_asset.Get());

            AssetBuilder::BuildAssetSettings settings;
            settings.m_assetFile = assetFilePath;
            if (build->m_sourceRoot)
                settings.m_sourceRoot = build->m_sourceRoot.Get();

            settings.m_outputDirectory = build->m_output.Get();
            return AssetBuilder::BuildAsset(settings) ? 0 : 1;
        }

        PrintHelp(m_cli);
        return 0;
    }
} // namespace FE::FerrumCli
