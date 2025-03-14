#pragma once
#include <Graphics/Core/Buffer.h>
#include <Graphics/Core/DrawArguments.h>
#include <Graphics/Core/GraphicsPipeline.h>
#include <Graphics/Core/InputStreamLayout.h>

namespace FE::Graphics::Core
{
    struct GraphicsPipeline;
    struct ShaderResourceGroup;


    enum class IndexType : uint32_t
    {
        kUint16 = 0,
        kUint32 = 1,
    };


    FE_FORCE_INLINE uint32_t GetIndexByteSize(const IndexType indexType)
    {
        return 2 + static_cast<uint32_t>(indexType) * 2;
    }


    struct IndexBufferView final
    {
        const Buffer* m_buffer;
        uint32_t m_byteOffset;
        uint32_t m_byteSize : 30;
        IndexType m_indexType : 2;

        [[nodiscard]] uint64_t GetHash() const
        {
            return DefaultHash(this, sizeof(*this));
        }
    };


    struct StreamBufferView final
    {
        const Buffer* m_buffer;
        uint32_t m_byteOffset;
        uint32_t m_byteSize;

        [[nodiscard]] uint64_t GetHash() const
        {
            return DefaultHash(this, sizeof(*this));
        }
    };


    struct GeometryView final
    {
        DrawArguments m_drawArguments;
        IndexBufferView m_indexBufferView;
        const StreamBufferView* m_streamBufferViews;

        [[nodiscard]] uint64_t GetHash(const uint32_t streamCount) const
        {
            Hasher hasher;
            hasher.UpdateRaw(m_drawArguments.GetHash());
            hasher.UpdateRaw(m_indexBufferView.GetHash());

            for (uint32_t streamIndex = 0; streamIndex < streamCount; ++streamIndex)
                hasher.UpdateRaw(m_streamBufferViews[streamIndex].GetHash());

            return hasher.Finalize();
        }
    };


    struct DrawCall final
    {
        uint32_t m_instanceOffset : 16;
        uint32_t m_instanceCount : 16;
        uint32_t m_stencilRef : 8;
        uint32_t m_shaderResourceGroupCount : 4;
        uint32_t m_unused : 20;
        const GeometryView* m_geometryView;
        const GraphicsPipeline* m_pipeline;
        const ShaderResourceGroup* const* m_shaderResourceGroups;

#if FE_DEBUG
        DrawCall()
        {
            memset(this, 0xcc, sizeof(*this));
        }
#endif

        void InitForSingleInstance(const GeometryView* geometryView, const GraphicsPipeline* pipeline)
        {
            m_instanceOffset = 0;
            m_instanceCount = 1;
            m_stencilRef = 0;
            m_shaderResourceGroupCount = 0;
            m_geometryView = geometryView;
            m_pipeline = pipeline;
            m_shaderResourceGroups = nullptr;
        }

        [[nodiscard]] uint64_t GetHash() const
        {
            Hasher hasher;
            hasher.Update(m_instanceOffset);
            hasher.Update(m_instanceCount);
            hasher.Update(m_stencilRef);
            hasher.Update(m_shaderResourceGroupCount);

            const auto& pipelineDesc = m_pipeline->GetDesc();
            hasher.UpdateRaw(m_geometryView->GetHash(pipelineDesc.m_inputLayout.CalculateActiveStreamCount()));

            //
            // Shader resource groups and pipelines are created using special factories.
            // Therefore, they are guaranteed to be unique, so we can hash the pointers rather than their descriptions.
            //

            hasher.Update(m_pipeline);

            for (uint32_t i = 0; i < m_shaderResourceGroupCount; ++i)
                hasher.Update(m_shaderResourceGroups[i]);

            return hasher.Finalize();
        }
    };


    struct DrawList final
    {
        uint32_t m_drawCallCount : 8;
        uint32_t m_sharedShaderResourceGroupCount : 4;
        uint32_t m_unused : 20;
        const DrawCall* m_drawCalls;
        const ShaderResourceGroup* const* m_sharedShaderResourceGroups;
    };
} // namespace FE::Graphics::Core
