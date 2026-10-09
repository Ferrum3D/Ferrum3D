#include <Core/Env/Environment.h>
#include <Core/RTTI/Reflection.h>
#include <GameFramework/Reflection.gen.h>

// Deliberately reference no GameFramework type, constructor or system function; the anchor is the only module reference.
int main(int argc, const char** argv)
{
    FE::CallLinkerAnchor_GameFramework();
    FE::Env::ApplicationInfo info;
    info.m_name = "GameFrameworkReflectionLinkTests";
    FE::Env::Init(info, argc, argv);
    const char* names[] = { "FE::GameFramework::TransformationSystem",
                            "FE::GameFramework::CameraSystem",
                            "FE::GameFramework::MeshSystem",
                            "FE::GameFramework::MeshComponent",
                            "FE::GameFramework::WorldStreamingService",
                            "FE::GameFramework::WorldGraphicsSceneService" };
    int32_t exitCode = 0;
    for (const char* name : names)
    {
        if (!FE::Rtti::TypeRegistry::FindType(name))
            exitCode = 1;
    }

    FE::Env::Shutdown();
    return exitCode;
}
