#include <FeCore/Assets/AssetManager.h>
#include <FeCore/Assets/AssetProviderDev.h>
#include <FeCore/Assets/IAssetLoader.h>

namespace FE::Assets
{
    extern "C"
    {
        FE_DLL_EXPORT IAssetManager* IAssetManager_CreateDevelopmentMode(const char* assetIndexFileName)
        {
            auto assetManager  = MakeShared<AssetManager>();
            auto assetProvider = MakeShared<AssetProviderDev>();
            auto assetRegistry = MakeShared<AssetRegistry>();
            assetRegistry->LoadAssetsFromFile(assetIndexFileName);
            assetProvider->AttachRegistry(assetRegistry);
            assetManager->AttachAssetProvider(static_pointer_cast<IAssetProvider>(assetProvider));
            return assetManager.Detach();
        }

        FE_DLL_EXPORT void IAssetManager_Destruct(IAssetManager* self)
        {
            self->ReleaseStrongRef();
        }
    }
} // namespace FE::Assets
