#pragma once
#include <FeCore/Assets/AssetStorage.h>
#include <FeCore/Containers/ArraySlice.h>
#include <OsAssets/Meshes/MeshLoaderImpl.h>

namespace FE::Osmium
{
    class MeshAssetStorage : public Assets::AssetStorage
    {
        friend class MeshAssetLoader;

        List<Float32> m_VertexBuffer;
        List<UInt32> m_IndexBuffer;
        List<MeshVertexComponent> m_Components;

        explicit MeshAssetStorage(MeshAssetLoader* loader);

    protected:
        void Delete() override;

    public:
        FE_CLASS_RTTI(MeshAssetStorage, "A34FEF78-A485-4B5D-969C-CFA52B949C9C");

        [[nodiscard]] inline ArraySlice<MeshVertexComponent> VertexComponents() const noexcept
        {
            return m_Components;
        }

        [[nodiscard]] inline UInt32 VertexSize() const noexcept
        {
            return static_cast<UInt32>(m_VertexBuffer.Size() * sizeof(Float32));
        }

        [[nodiscard]] inline UInt32 IndexSize() const noexcept
        {
            return static_cast<UInt32>(m_IndexBuffer.Size() * sizeof(UInt32));
        }

        [[nodiscard]] inline const Float32* VertexData() const noexcept
        {
            return m_VertexBuffer.Data();
        }

        [[nodiscard]] inline const UInt32* IndexData() const noexcept
        {
            return m_IndexBuffer.Data();
        }
    };
} // namespace FE::Osmium
