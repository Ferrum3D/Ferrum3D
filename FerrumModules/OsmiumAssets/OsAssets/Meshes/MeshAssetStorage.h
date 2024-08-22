#pragma once
#include <FeCore/Assets/AssetStorage.h>
#include <OsAssets/Meshes/MeshLoaderImpl.h>

namespace FE::Osmium
{
    class MeshAssetStorage : public Assets::AssetStorage
    {
        friend class MeshAssetLoader;

        eastl::vector<float> m_VertexBuffer;
        eastl::vector<uint32_t> m_IndexBuffer;
        eastl::vector<MeshVertexComponent> m_Components;

    protected:
        void Delete() override;

    public:
        explicit MeshAssetStorage(MeshAssetLoader* loader);

        FE_RTTI_Class(MeshAssetStorage, "A34FEF78-A485-4B5D-969C-CFA52B949C9C");

        [[nodiscard]] inline festd::span<const MeshVertexComponent> VertexComponents() const noexcept
        {
            return m_Components;
        }

        [[nodiscard]] inline uint32_t VertexSize() const noexcept
        {
            return static_cast<uint32_t>(m_VertexBuffer.size() * sizeof(float));
        }

        [[nodiscard]] inline uint32_t IndexSize() const noexcept
        {
            return static_cast<uint32_t>(m_IndexBuffer.size() * sizeof(uint32_t));
        }

        [[nodiscard]] inline const float* VertexData() const noexcept
        {
            return m_VertexBuffer.data();
        }

        [[nodiscard]] inline const uint32_t* IndexData() const noexcept
        {
            return m_IndexBuffer.data();
        }
    };
} // namespace FE::Osmium
