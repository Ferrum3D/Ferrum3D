#include <Graphics/Assets/ShaderAssetLoader.h>
#include <Graphics/Assets/ShaderAssetStorage.h>

namespace FE::Graphics
{
    ShaderAssetStorage::ShaderAssetStorage(ShaderAssetLoader* loader)
        : Assets::AssetStorage(loader)
    {
    }


    void ShaderAssetStorage::Delete()
    {
        m_ShaderModule.Reset();
    }
} // namespace FE::Graphics
