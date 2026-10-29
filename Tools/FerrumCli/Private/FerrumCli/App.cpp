#include <FerrumCli/App.h>

#include <AssetBuilder/AssetDepot.h>
#include <AssetBuilder/AssetPipeline.h>
#include <AssetBuilder/AssetServer.h>
#include <AssetBuilder/ShaderProcessor.h>
#include <Core/Base/PlatformInclude.h>
#include <Core/IO/BaseIO.h>
#include <Core/Strings/Encoding.h>
#include <Core/Threading/Thread.h>

namespace FE::FerrumCli
{
    namespace
    {
        //! Interval between polls of a running asset server.
        constexpr uint32_t kServerPollMilliseconds = 50;

        std::atomic<bool> GStopRequested = false;


        BOOL WINAPI HandleConsoleControl(const DWORD controlType)
        {
            if (controlType != CTRL_C_EVENT && controlType != CTRL_BREAK_EVENT && controlType != CTRL_CLOSE_EVENT)
                return FALSE;

            GStopRequested.store(true);
            return TRUE;
        }


        void ParseDepotList(const festd::string_view list, festd::vector<IO::Path>& directories)
        {
            uint32_t start = 0;
            for (uint32_t index = 0; index <= list.size(); ++index)
            {
                if (index < list.size() && list.byte_at(index) != ';')
                    continue;

                if (index > start)
                    directories.push_back(IO::Path{ list.substr_ascii(start, index - start) });

                start = index + 1;
            }
        }
    } // namespace


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

        if (const Serve* serve = m_cli.GetSubcommand<Serve>())
            return RunServe(*serve);

        if (const Status* status = m_cli.GetSubcommand<Status>())
            return RunStatus(*status);

        if (const BuildShaderCache* buildShaderCache = m_cli.GetSubcommand<BuildShaderCache>())
            return RunBuildShaderCache(*buildShaderCache);

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

        if (command.m_all)
            return RunBuildAll(command);

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


    bool App::ScanDepot(const Cli::Command& command, const Cli::Option& depot, const Cli::Option& sourceRoot,
                        const Cli::Option& output, AssetBuilder::AssetDepot& result)
    {
        if (!depot || depot.Get().empty())
        {
            IO::EPrintLn("Error: Option '--depot' is required");
            PrintHelp(command);
            return false;
        }

        if (!output || output.Get().empty())
        {
            IO::EPrintLn("Error: Option '--output' is required");
            PrintHelp(command);
            return false;
        }

        AssetBuilder::DepotSettings settings;
        ParseDepotList(depot.Get(), settings.m_depotDirectories);
        settings.m_outputDirectory = output.Get();
        if (sourceRoot)
        {
            if (settings.m_depotDirectories.size() > 1)
            {
                IO::EPrintLn("Error: Option '--source-root' cannot be used with several depots");
                return false;
            }

            settings.m_sourceRoot = sourceRoot.Get();
        }

        return result.Scan(settings);
    }


    int32_t App::RunBuildAll(const Build& command)
    {
        if (command.m_asset)
        {
            IO::EPrintLn("Error: Options '--all' and '--asset' cannot be combined");
            return 1;
        }

        AssetBuilder::AssetDepot depot;
        if (!ScanDepot(command, command.m_depot, command.m_sourceRoot, command.m_output, depot))
            return 1;

        festd::vector<uint32_t> assets;
        depot.GetAllAssets(assets);
        const AssetBuilder::DepotBuildResult result =
            AssetBuilder::BuildDepotAssets(depot, festd::span{ assets.data(), assets.size() });

        IO::PrintLn("{} assets up to date or built, {} failed, {} skipped, {} products published",
                    result.m_succeededCount,
                    result.m_failedCount,
                    result.m_skippedCount,
                    result.m_publishedProducts.size());
        return result.IsSuccessful() ? 0 : 1;
    }


    int32_t App::RunServe(const Serve& command)
    {
        if (command.m_help)
        {
            PrintHelp(command);
            return 0;
        }

        AssetBuilder::AssetDepot depot;
        if (!ScanDepot(command, command.m_depot, command.m_sourceRoot, command.m_output, depot))
            return 1;

        AssetBuilder::AssetServerSettings settings;
        settings.m_depot = depot.GetSettings();
        settings.m_fullScan = static_cast<bool>(command.m_fullScan);
        settings.m_useChangeJournal = !command.m_noJournal;

        AssetBuilder::AssetServer server{ settings };
        if (!server.Start())
            return 1;

        IO::PrintLn("Serving {} assets ({}); press Ctrl+C to stop",
                    server.GetDepot().GetAssets().size(),
                    server.IsUsingChangeJournal() ? "change journal" : "directory watchers");
        IO::Flush(IO::StandardDescriptor::kStdout);

        SetConsoleCtrlHandler(HandleConsoleControl, TRUE);
        while (!GStopRequested.load())
        {
            server.Poll();
            Threading::Sleep(kServerPollMilliseconds);
        }

        SetConsoleCtrlHandler(HandleConsoleControl, FALSE);
        return 0;
    }


    int32_t App::RunStatus(const Status& command)
    {
        if (command.m_help)
        {
            PrintHelp(command);
            return 0;
        }

        if (!command.m_output || command.m_output.Get().empty())
        {
            IO::EPrintLn("Error: Option '--output' is required");
            PrintHelp(command);
            return 1;
        }

        const festd::string pipeName = AssetBuilder::GetAssetServerPipeName(IO::Path{ command.m_output.Get() });
        const Str::Utf8ToUtf16 widePipeName{ pipeName.data(), pipeName.size() };
        const HANDLE pipe = CreateFileW(widePipeName.ToWideString(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);
        if (pipe == INVALID_HANDLE_VALUE)
        {
            IO::EPrintLn("Error: No asset server is building into '{}'", command.m_output.Get());
            return 1;
        }

        // The server closes the pipe when it stops.
        char buffer[4096];
        DWORD bytesRead = 0;
        while (ReadFile(pipe, buffer, sizeof(buffer), &bytesRead, nullptr) && bytesRead > 0)
        {
            IO::Print("{}", festd::string_view{ buffer, bytesRead });
            IO::Flush(IO::StandardDescriptor::kStdout);
        }

        CloseHandle(pipe);
        return 0;
    }


    int32_t App::RunBuildShaderCache(const BuildShaderCache& command)
    {
        if (command.m_help)
        {
            PrintHelp(command);
            return 0;
        }

        AssetBuilder::AssetDepot depot;
        if (!ScanDepot(command, command.m_depot, command.m_sourceRoot, command.m_output, depot))
            return 1;

        const auto config =
            command.m_debug ? Graphics::Core::ShaderCompilerConfig::kDebug : Graphics::Core::ShaderCompilerConfig::kOptimized;
        return AssetBuilder::BuildShaderCache(depot, config) ? 0 : 1;
    }
} // namespace FE::FerrumCli
