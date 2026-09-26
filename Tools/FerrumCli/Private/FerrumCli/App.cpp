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
            return RunImport(*import);

        if (const Build* build = m_cli.GetSubcommand<Build>())
            return RunBuild(*build);

        PrintHelp(m_cli);
        return 0;
    }


    int32_t App::RunImport(const Import& command)
    {
        if (command.m_help)
        {
            PrintHelp(command);
            return 0;
        }

        const bool hasSourceAsset = command.m_asset && !command.m_asset.Get().empty();
        const bool hasAssetType = command.m_assetType && !command.m_assetType.Get().empty();
        if (hasSourceAsset == hasAssetType)
        {
            IO::EPrintLn("Error: Specify exactly one of '--asset' or '--asset-type'");
            PrintHelp(command);
            return 1;
        }

        IO::Path outputPath;
        if (command.m_output)
        {
            outputPath = command.m_output.Get();
        }
        else if (hasSourceAsset)
        {
            const IO::PathView sourcePath(command.m_asset.Get());
            outputPath = sourcePath.parent_directory();
            IO::Path outputName(sourcePath.stem());
            outputName.AsBaseString() += ".asset";
            outputPath /= outputName;
        }
        else
        {
            IO::EPrintLn("Error: Option '--output' is required with '--asset-type'");
            PrintHelp(command);
            return 1;
        }

        AssetBuilder::ImportAssetSettings settings;
        if (hasSourceAsset)
        {
            settings.m_inputFile = command.m_asset.Get();
        }
        else
        {
            const festd::string_view assetType = command.m_assetType.Get();
            settings.m_assetTypeId = Rtti::TypeID(festd::ascii_view{ assetType.data(), assetType.size() });
            if (!settings.m_assetTypeId.IsValid())
            {
                IO::EPrintLn("Error: Option '--asset-type' must be a valid, nonzero UUID");
                return 1;
            }
        }

        settings.m_outputFile = outputPath;
        return AssetBuilder::ImportAsset(settings) ? 0 : 1;
    }


    int32_t App::RunBuild(const Build& command)
    {
        if (command.m_help)
        {
            PrintHelp(command);
            return 0;
        }

        if (!command.m_asset || command.m_asset.Get().empty())
        {
            IO::EPrintLn("Error: Option '--asset' is required");
            PrintHelp(command);
            return 1;
        }

        if (!command.m_output || command.m_output.Get().empty())
        {
            IO::EPrintLn("Error: Option '--output' is required");
            PrintHelp(command);
            return 1;
        }

        const IO::PathView assetFilePath(command.m_asset.Get());

        AssetBuilder::BuildAssetSettings settings;
        settings.m_assetFile = assetFilePath;
        if (command.m_sourceRoot)
            settings.m_sourceRoot = command.m_sourceRoot.Get();

        settings.m_outputDirectory = command.m_output.Get();
        return AssetBuilder::BuildAsset(settings) ? 0 : 1;
    }
} // namespace FE::FerrumCli
