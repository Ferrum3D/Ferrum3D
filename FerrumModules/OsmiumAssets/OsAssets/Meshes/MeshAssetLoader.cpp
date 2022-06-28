#include <OsAssets/Meshes/MeshAssetLoader.h>
#include <OsAssets/Meshes/MeshAssetStorage.h>
#include <OsAssets/Meshes/MeshLoaderImpl.h>
#include <FeCore/Console/FeLog.h>

namespace FE::Osmium
{
    Assets::AssetType MeshAssetLoader::AssetType = Assets::AssetType("77ADC20F-B033-4B55-8498-48B59BB92C08");

    Assets::AssetType MeshAssetLoader::GetAssetType() const
    {
        return AssetType;
    }

    Assets::AssetStorage* MeshAssetLoader::CreateStorage()
    {
        auto* p = GlobalAllocator<HeapAllocator>::Get().Allocate(sizeof(MeshAssetStorage), MaximumAlignment, FE_SRCPOS());
        return new (p) MeshAssetStorage(this);
    }

    void Osmium::MeshAssetLoader::LoadAsset(Assets::AssetStorage* storage, IO::IStream* assetStream)
    {
        List<MeshVertexComponent> components = {
            MeshVertexComponent::Position3F,
            MeshVertexComponent::TextureCoordinate2F
        };

        auto* imageStorage = static_cast<MeshAssetStorage*>(storage);
        auto length        = assetStream->Length();
        List<Int8> buffer(length, 0);
        auto success = LoadMeshFromMemory(buffer, components, imageStorage->m_VertexBuffer, imageStorage->m_IndexBuffer, imageStorage->m_VertexCount);
        FE_ASSERT_MSG(success, "Failed to load a mesh");
    }
} // namespace FE::Osmium
