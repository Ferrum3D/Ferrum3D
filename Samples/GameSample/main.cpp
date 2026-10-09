#include "SampleAssets.h"
#include <Core/IO/BaseIO.h>
#include <Core/Jobs/Jobs.h>
#include <GameFramework/Application.h>

using namespace FE;

int main(int argc, const char** argv)
{
    Env::ApplicationInfo info;
    info.m_name = "Ferrum3D - Game Sample";
    Env::Init(info, argc, argv);

    // The shader cache resolves engine sources relative to the sample build directory.
    IO::Directory::SetCurrentDirectory(FE_GAME_SAMPLE_RUNTIME_DIR);
    auto* application = Memory::DefaultNew<GameFramework::Application>();
    GameFramework::ApplicationSettings settings;
    settings.m_assetDirectory = IO::Path(FE_GAME_SAMPLE_ASSET_DIR);
    settings.m_world = GameSample::kWorld;
    int32_t exitCode = 0;
    const bool smoke = argc > 1 && strcmp(argv[1], "--smoke") == 0;
    Jobs::DispatchMainThread([&] {
        if (application->Initialize(settings))
        {
            exitCode = application->Run(smoke ? 8 : 0);
            if (smoke)
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
