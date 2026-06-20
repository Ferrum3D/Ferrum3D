#include <FerrumCli/App.h>

#include <AssetBuilder/ModelProcessor.h>
#include <AssetBuilder/TextureProcessor.h>
#include <Core/DI/Builder.h>
#include <Core/IO/IStreamFactory.h>
#include <Core/Modules/Configuration.h>

namespace FE::FerrumCli
{
    App::App(const festd::span<const festd::string_view> commandLine)
    {
        DI::ServiceRegistryBuilder builder{ Env::GetRootServiceRegistry() };
        builder.Bind<Env::Configuration>()
            .ToFunc([commandLine](DI::IServiceProvider*, Memory::RefCountedObjectBase** result) {
                std::pmr::memory_resource* allocator = Env::GetStaticAllocator(Memory::StaticAllocatorType::kLinear);
                *result = Rc<Env::Configuration>::New(allocator, commandLine);
                return DI::ResultCode::kSuccess;
            })
            .InSingletonScope();
        builder.Build();

        DI::IServiceProvider* serviceProvider = Env::GetServiceProvider();
        m_logger = serviceProvider->ResolveRequired<Logger>();
        m_logSink = festd::make_unique<Framework::StdoutLogSink>(m_logger.Get());

        std::pmr::memory_resource* allocator = Env::GetStaticAllocator(Memory::StaticAllocatorType::kLinear);
        m_cli = Cli::Parse<CommandLineParser>(allocator, commandLine);
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

        const Build* build = m_cli.GetSubcommand<Build>();
        if (build == nullptr)
        {
            PrintHelp(m_cli);
            return 0;
        }

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

        DI::IServiceProvider* serviceProvider = Env::GetServiceProvider();
        IO::IStreamFactory* streamFactory = serviceProvider->ResolveRequired<IO::IStreamFactory>();

        const festd::string_view assetPath = build->m_asset.Get();
        const IO::PathView pathView{ assetPath };
        const IO::Path fullInputPath = IO::GetAbsolutePath(assetPath);

        IO::Path outputPath = pathView.parent_directory();
        outputPath /= pathView.stem();

        bool succeeded;
        if (pathView.extension() == ".glb")
        {
            AssetBuilder::ModelProcessSettings settings;
            settings.m_logger = m_logger.Get();
            settings.m_streamFactory = streamFactory;
            settings.m_inputFile = fullInputPath;
            settings.m_outputFile = outputPath;
            settings.m_outputFile.append(".fmd");
            succeeded = AssetBuilder::ProcessModel(settings);
        }
        else
        {
            AssetBuilder::TextureProcessSettings settings;
            settings.m_logger = m_logger.Get();
            settings.m_streamFactory = streamFactory;
            settings.m_inputFile = fullInputPath;
            settings.m_outputFile = outputPath;
            settings.m_outputFile.append(".ftx");
            succeeded = AssetBuilder::ProcessTexture(settings);
        }

        return succeeded ? 0 : 1;
    }
} // namespace FE::FerrumCli