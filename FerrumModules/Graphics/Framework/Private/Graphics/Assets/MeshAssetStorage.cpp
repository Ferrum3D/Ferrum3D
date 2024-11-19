#include <Graphics/Assets/MeshAssetLoader.h>
#include <Graphics/Assets/MeshAssetStorage.h>

namespace FE::Graphics
{
    MeshAssetStorage::MeshAssetStorage(MeshAssetLoader* loader)
        : Assets::AssetStorage(loader)
    {
    }


    void MeshAssetStorage::Delete()
    {
        m_vertexBuffer.Reset();
        m_indexBuffer.Reset();
    }
} // namespace FE::Graphics
