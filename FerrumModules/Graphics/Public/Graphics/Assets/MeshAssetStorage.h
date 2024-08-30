#pragma once
#include <FeCore/Assets/AssetStorage.h>
#include <HAL/Buffer.h>

namespace FE::Graphics
{
    class MeshAssetLoader;

    class MeshAssetStorage : public Assets::AssetStorage
    {
        friend class MeshAssetLoader;

        Rc<HAL::Buffer> m_indexBuffer;
        Rc<HAL::Buffer> m_vertexBuffer;

    protected:
        void Delete() override;

    public:
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

        [[nodiscard]] HAL::Buffer* GetIndexBuffer() const
        {
            return m_indexBuffer.Get();
        }

        [[nodiscard]] HAL::Buffer* GetVertexBuffer() const
        {
            return m_vertexBuffer.Get();
        }

        [[nodiscard]] uint32_t GetIndexCount() const
        {
            return static_cast<uint32_t>(m_indexBuffer->GetDesc().Size / sizeof(uint32_t));
        }
    };
} // namespace FE::Graphics
