#include <FeCore/Assets/IAssetManager.h>
#include <FeCore/Modules/Environment.h>
#include <FeCore/Modules/Singleton.h>
#include <OsAssets/Images/ImageAssetLoader.h>
#include <OsAssets/Meshes/MeshAssetLoader.h>

namespace FE::Osmium
{
    extern "C"
    {
        FE_DLL_EXPORT void AttachEnvironment(FE::Env::Internal::IEnvironment* environment)
        {
            Env::AttachEnvironment(*environment);
            auto manager = Singleton<Assets::IAssetManager>::Get();
            manager->RegisterAssetLoader(static_pointer_cast<Assets::IAssetLoader>(MakeShared<ImageAssetLoader>()));
            manager->RegisterAssetLoader(static_pointer_cast<Assets::IAssetLoader>(MakeShared<MeshAssetLoader>()));
        }

        FE_DLL_EXPORT void DetachEnvironment()
        {
            auto manager = Singleton<Assets::IAssetManager>::Get();
            manager->RemoveAssetLoader(ImageAssetLoader::AssetType);
            manager->RemoveAssetLoader(MeshAssetLoader::AssetType);
        }
    }
} // namespace FE::Osmium
