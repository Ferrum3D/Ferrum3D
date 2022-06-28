#pragma once
#include <FeCore/Assets/AssetStorage.h>

namespace FE::Osmium
{
    class MeshAssetStorage : public Assets::AssetStorage
    {
        friend class MeshAssetLoader;

        List<Float32> m_VertexBuffer;
        UInt32 m_VertexCount;
        List<UInt32> m_IndexBuffer;

        explicit MeshAssetStorage(MeshAssetLoader* loader);

    protected:
        void Delete() override;

    public:
        FE_CLASS_RTTI(MeshAssetStorage, "A34FEF78-A485-4B5D-969C-CFA52B949C9C");

        [[nodiscard]] Assets::AssetType GetAssetType() const override;
    };
} // namespace FE::Osmium
