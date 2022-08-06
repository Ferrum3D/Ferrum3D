#include <OsAssets/Shaders/ShaderAssetLoader.h>
#include <OsAssets/Shaders/ShaderAssetStorage.h>

namespace FE::Osmium
{
    ShaderAssetStorage::ShaderAssetStorage(ShaderAssetLoader* loader)
        : Assets::AssetStorage(loader)
    {
    }

    void ShaderAssetStorage::Delete()
    {
        SourceCode.Clear();
    }
} // namespace FE::Osmium
