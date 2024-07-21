#pragma once
#include <FeCore/Assets/AssetStorage.h>
#include <FeCore/Containers/ArraySlice.h>
#include <OsAssets/Meshes/MeshLoaderImpl.h>

namespace FE::Osmium
{
    class MeshAssetStorage : public Assets::AssetStorage
    {
        friend class MeshAssetLoader;

        eastl::vector<float> m_VertexBuffer;
        eastl::vector<UInt32> m_IndexBuffer;
        eastl::vector<MeshVertexComponent> m_Components;

    protected:
        void Delete() override;

    public:
        explicit MeshAssetStorage(MeshAssetLoader* loader);

        FE_CLASS_RTTI(MeshAssetStorage, "A34FEF78-A485-4B5D-969C-CFA52B949C9C");

        [[nodiscard]] inline ArraySlice<MeshVertexComponent> VertexComponents() const noexcept
        {
            return m_Components;
        }

        [[nodiscard]] inline UInt32 VertexSize() const noexcept
        {
            return static_cast<UInt32>(m_VertexBuffer.size() * sizeof(float));
        }

        [[nodiscard]] inline UInt32 IndexSize() const noexcept
        {
            return static_cast<UInt32>(m_IndexBuffer.size() * sizeof(UInt32));
        }

        [[nodiscard]] inline const float* VertexData() const noexcept
        {
            return m_VertexBuffer.data();
        }

        [[nodiscard]] inline const UInt32* IndexData() const noexcept
        {
            return m_IndexBuffer.data();
        }
    };
} // namespace FE::Osmium
