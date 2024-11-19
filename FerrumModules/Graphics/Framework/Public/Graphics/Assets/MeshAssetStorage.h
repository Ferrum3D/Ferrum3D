#pragma once
#include <FeCore/Assets/AssetStorage.h>
#include <Graphics/RHI/Buffer.h>

namespace FE::Graphics
{
    struct MeshAssetLoader;

    struct MeshAssetStorage final : public Assets::AssetStorage
    {
        explicit MeshAssetStorage(MeshAssetLoader* loader);

        FE_RTTI_Class(MeshAssetStorage, "A34FEF78-A485-4B5D-969C-CFA52B949C9C");

        static constexpr std::string_view kAssetTypeName = "Mesh";

        [[nodiscard]] bool IsAnythingLoaded() const override
        {
            return true;
        }

        [[nodiscard]] bool IsCompletelyLoaded() const override
        {
            return true;
        }

        [[nodiscard]] RHI::Buffer* GetIndexBuffer() const
        {
            return m_indexBuffer.Get();
        }

        [[nodiscard]] RHI::Buffer* GetVertexBuffer() const
        {
            return m_vertexBuffer.Get();
        }

        [[nodiscard]] uint32_t GetIndexCount() const
        {
            return static_cast<uint32_t>(m_indexBuffer->GetDesc().m_size / sizeof(uint32_t));
        }

    private:
        friend struct MeshAssetLoader;

        Rc<RHI::Buffer> m_indexBuffer;
        Rc<RHI::Buffer> m_vertexBuffer;

    protected:
        void Delete() override;
    };
} // namespace FE::Graphics
