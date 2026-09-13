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

        const Import* import = m_cli.GetSubcommand<Import>();
        const Build* build = m_cli.GetSubcommand<Build>();
        if (import == nullptr && build == nullptr)
        {
            PrintHelp(m_cli);
            return 0;
        }

        if (import != nullptr)
        {
            if (import->m_help)
            {
                PrintHelp(*import);
                return 0;
            }
            if (!import->m_asset || import->m_asset.Get().empty())
            {
                IO::EPrintLn("Error: Option '--asset' is required");
                PrintHelp(*import);
                return 1;
            }

            const IO::PathView sourcePath(import->m_asset.Get());
            IO::Path outputPath;
            if (import->m_output)
                outputPath = import->m_output.Get();
            else
            {
                outputPath = sourcePath.parent_directory();
                IO::Path outputName(sourcePath.stem());
                outputName.AsBaseString() += ".asset";
                outputPath /= outputName;
            }

            AssetBuilder::ImportAssetSettings settings;
            settings.m_inputFile = sourcePath;
            settings.m_outputFile = outputPath;
            return AssetBuilder::ImportAsset(settings) ? 0 : 1;
        }

        FE_Assert(build != nullptr);
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

        const IO::PathView assetFilePath(build->m_asset.Get());
        IO::Path outputPath;
        if (build->m_output)
        {
            outputPath = build->m_output.Get();
        }
        else
        {
            outputPath = assetFilePath.parent_directory();
            outputPath /= assetFilePath.stem();
        }

        AssetBuilder::BuildAssetSettings settings;
        settings.m_assetFile = assetFilePath;
        settings.m_outputDirectory = outputPath;
        return AssetBuilder::BuildAsset(settings) ? 0 : 1;
    }
} // namespace FE::FerrumCli
