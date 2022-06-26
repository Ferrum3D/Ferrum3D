#include <FeCore/Modules/Singleton.h>
#include <FeCore/Modules/Environment.h>
#include <FeCore/Assets/IAssetManager.h>
#include <OsAssets/Images/ImageAssetLoader.h>

namespace FE::Osmium
{
    extern "C"
    {
        FE_DLL_EXPORT void AttachEnvironment(FE::Env::Internal::IEnvironment* environment)
        {
            Env::AttachEnvironment(*environment);
            auto manager = Singleton<Assets::IAssetManager>::Get();
            manager->RegisterAssetLoader(static_pointer_cast<Assets::IAssetLoader>(MakeShared<ImageAssetLoader>()));
        }
    }
}
