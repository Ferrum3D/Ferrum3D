#pragma once
#include <Graphics/Core/Buffer.h>
#include <Graphics/Core/DrawArguments.h>
#include <Graphics/Core/GraphicsPipeline.h>
#include <Graphics/Core/InputStreamLayout.h>

namespace FE::Graphics::Core
{
    struct GraphicsPipeline;


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
        uint32_t m_unused : 24;
        const GeometryView* m_geometryView;
        const GraphicsPipeline* m_pipeline;

        void InitForSingleInstance(const GeometryView* geometryView, const GraphicsPipeline* pipeline)
        {
            m_instanceOffset = 0;
            m_instanceCount = 1;
            m_stencilRef = 0;
            m_geometryView = geometryView;
            m_pipeline = pipeline;
        }

        [[nodiscard]] uint64_t GetHash() const
        {
            Hasher hasher;
            hasher.Update(m_instanceOffset);
            hasher.Update(m_instanceCount);
            hasher.Update(m_stencilRef);

            const auto& pipelineDesc = m_pipeline->GetDesc();
            hasher.UpdateRaw(m_geometryView->GetHash(pipelineDesc.m_inputLayout.CalculateActiveStreamCount()));

            //
            // Pipelines are created by the pipeline factory.
            // Therefore, they are guaranteed to be unique, so we can hash the pointers rather than their descriptions.
            //

            hasher.Update(m_pipeline);

            return hasher.Finalize();
        }
    };


    struct DrawList final
    {
        uint32_t m_drawCallCount;
        const DrawCall* m_drawCalls;
    };
} // namespace FE::Graphics::Core
