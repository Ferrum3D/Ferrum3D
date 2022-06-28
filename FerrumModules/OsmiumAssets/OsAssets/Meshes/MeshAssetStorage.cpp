#include <OsAssets/Meshes/MeshAssetLoader.h>
#include <OsAssets/Meshes/MeshAssetStorage.h>

namespace FE::Osmium
{
    MeshAssetStorage::MeshAssetStorage(MeshAssetLoader* loader)
        : Assets::AssetStorage(loader)
    {
    }

    Assets::AssetType MeshAssetStorage::GetAssetType() const
    {
        return MeshAssetLoader::AssetType;
    }

    void MeshAssetStorage::Delete()
    {
        m_VertexBuffer.Clear();
        m_IndexBuffer.Clear();
    }
} // namespace FE::Osmium
