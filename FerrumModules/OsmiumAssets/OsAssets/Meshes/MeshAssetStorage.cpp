#include <OsAssets/Meshes/MeshAssetLoader.h>
#include <OsAssets/Meshes/MeshAssetStorage.h>

namespace FE::Osmium
{
    MeshAssetStorage::MeshAssetStorage(MeshAssetLoader* loader)
        : Assets::AssetStorage(loader)
    {
    }

    void MeshAssetStorage::Delete()
    {
        m_VertexBuffer.clear();
        m_IndexBuffer.clear();
    }
} // namespace FE::Osmium
