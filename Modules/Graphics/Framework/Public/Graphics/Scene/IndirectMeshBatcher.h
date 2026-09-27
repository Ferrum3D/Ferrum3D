#pragma once
#include <Graphics/Scene/RenderBatch.h>
#include <festd/vector.h>

namespace FE::Graphics::Core
{
    struct FrameGraph;
    struct RingUploader;
} // namespace FE::Graphics::Core

namespace FE::Graphics
{
    struct IndirectMeshGroup final
    {
        Core::GraphicsPipeline* m_pipeline = nullptr;
        BufferPointer m_meshInstanceTable;
        BufferPointer m_meshGroupTable;
        BufferPointer m_meshLodInfoTable;
        BufferPointer m_materialInstanceTable;
        uint32_t m_firstInstance = 0;
        uint32_t m_instanceCount = 0;
        uint32_t m_meshletCount = 0;
        uint32_t m_meshletX = 0;
        uint32_t m_argumentIndex = 0;
    };


    struct IndirectMeshBatcher final
    {
        explicit IndirectMeshBatcher(std::pmr::memory_resource* allocator)
            : m_groups(allocator)
        {
        }

        void Build(Core::FrameGraph& graph, Core::RingUploader& uploader, const SegmentedVector<RenderBatch>& batches);

        festd::pmr::vector<IndirectMeshGroup> m_groups;
        Rc<Core::Buffer> m_instanceIndices;
        Rc<Core::Buffer> m_arguments;
    };
} // namespace FE::Graphics
