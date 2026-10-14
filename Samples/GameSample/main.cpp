#include <Core/CLI/CommandLine.h>
#include <Core/IO/BaseIO.h>
#include <Core/Jobs/Jobs.h>
#include <GameFramework/Application.h>

using namespace FE;

static const IO::AssetID kWorldAssetId("AAA69126-4427-4055-B1C4-04013AAE10A0");

int main(const int argc, const char** argv)
{
    Env::ApplicationInfo info;
    info.m_name = "Ferrum3D - Game Sample";
    Env::Init(info, argc, argv);

    // The shader cache resolves engine sources relative to the sample build directory.
    IO::Directory::SetCurrentDirectory(FE_GAME_SAMPLE_RUNTIME_DIR);

    auto* application = Memory::DefaultNew<GameFramework::Application>();

    GameFramework::ApplicationSettings settings;
    settings.m_assetDirectory = FE_GAME_SAMPLE_ASSET_DIR;
    settings.m_world = kWorldAssetId;
    if (const auto debugMode = Cli::GetValue("--visibility-debug"))
        settings.m_visibilityDebugMode = Graphics::Visibility::ParseDebugMode(*debugMode);

    int32_t exitCode = 0;
    const bool smoke = Cli::Check("--smoke");
    Jobs::DispatchMainThread([&] {
        if (application->Initialize(settings))
        {
            exitCode = application->Run(smoke ? 8 : 0);
            if (smoke && exitCode == 0)
                Logger::LogInfo("GameSample rendered its disk world successfully");
        }
        else
        {
            exitCode = 1;
        }

        application->Shutdown();
        Jobs::StopJobSystem();
    });

    Jobs::StartJobSystem();
    Memory::DefaultDelete(application);
    Env::Shutdown();
    return exitCode;
}
