#include <Graphics/Core/FrameGraph/FrameGraph.h>
#include <Graphics/Core/PipelineVariantSet.h>
#include <Graphics/Core/RingUploader.h>
#include <Graphics/Scene/IndirectMeshBatcher.h>

#include <Shaders/Passes/MeshPass/BuildIndirectArgs.h>
#include <Shaders/Passes/MeshPass/IndirectArguments.h>

namespace FE::Graphics
{
    static_assert(sizeof(MeshPass::MeshDispatchArguments) == 3 * sizeof(uint32_t));


    namespace
    {
        struct IndirectArgsPipeline final : public Core::ComputePipelineVariantSet
        {
            FE_DECLARE_PIPELINE_SET(IndirectArgsPipeline);

        private:
            void SetupRequest([[maybe_unused]] uint32_t variantIndex, Core::ComputePipelineRequest& request) override
            {
                request.m_desc.SetComputeShader("Shaders/Passes/MeshPass/BuildIndirectArgs.cs.hlsl");
            }
        };
        FE_IMPLEMENT_PIPELINE_SET(IndirectArgsPipeline);


        bool SameGpuGroup(const RenderDraw& lhs, const RenderDraw& rhs)
        {
            return lhs.m_pipeline == rhs.m_pipeline && lhs.m_meshGroupIndex == rhs.m_meshGroupIndex
                && lhs.m_meshGroupTable.GetDeviceAddress() == rhs.m_meshGroupTable.GetDeviceAddress()
                && lhs.m_meshInstanceTable.GetDeviceAddress() == rhs.m_meshInstanceTable.GetDeviceAddress()
                && lhs.m_meshLodInfoTable.GetDeviceAddress() == rhs.m_meshLodInfoTable.GetDeviceAddress()
                && lhs.m_materialInstanceTable.GetDeviceAddress() == rhs.m_materialInstanceTable.GetDeviceAddress()
                && lhs.m_meshletCount == rhs.m_meshletCount;
        }
    } // namespace


    void IndirectMeshBatcher::Build(Core::FrameGraph& graph, Core::RingUploader& uploader,
                                    const SegmentedVector<RenderBatch>& batches)
    {
        festd::pmr::vector<const RenderDraw*> draws{ graph.GetAllocator() };
        for (const RenderBatch& batch : batches)
        {
            for (const RenderDraw& draw : batch.m_draws)
                draws.push_back(&draw);
        }

        if (draws.empty())
            return;

        festd::sort(draws, [](const RenderDraw* lhs, const RenderDraw* rhs) {
            const uintptr_t lhsPipeline = reinterpret_cast<uintptr_t>(lhs->m_pipeline);
            const uintptr_t rhsPipeline = reinterpret_cast<uintptr_t>(rhs->m_pipeline);
            if (lhsPipeline != rhsPipeline)
                return lhsPipeline < rhsPipeline;

            const uint64_t lhsTable = lhs->m_meshGroupTable.GetDeviceAddress();
            const uint64_t rhsTable = rhs->m_meshGroupTable.GetDeviceAddress();
            if (lhsTable != rhsTable)
                return lhsTable < rhsTable;

            return lhs->m_meshGroupIndex < rhs->m_meshGroupIndex;
        });

        festd::pmr::vector<uint32_t> instanceIndices{ graph.GetAllocator() };
        festd::pmr::vector<MeshPass::MeshDispatchArguments> groupCounts{ graph.GetAllocator() };

        uint32_t firstDraw = 0;
        while (firstDraw < draws.size())
        {
            const RenderDraw& first = *draws[firstDraw];
            uint32_t lastDraw = firstDraw + 1;
            while (lastDraw < draws.size() && SameGpuGroup(first, *draws[lastDraw]))
                ++lastDraw;

            constexpr uint32_t kMaxDimension = 65535;
            constexpr uint32_t kMaxTotalGroups = 1 << 22;
            const uint32_t meshletX = Math::Min(first.m_meshletCount, kMaxDimension);
            const uint32_t meshletZ = Math::CeilDivide(first.m_meshletCount, meshletX);
            const uint64_t groupsPerInstance = static_cast<uint64_t>(meshletX) * meshletZ;
            FE_Assert(groupsPerInstance <= kMaxTotalGroups);

            const uint32_t maxInstances = Math::Min(kMaxDimension, static_cast<uint32_t>(kMaxTotalGroups / groupsPerInstance));

            for (uint32_t pageStart = firstDraw; pageStart < lastDraw; pageStart += maxInstances)
            {
                const uint32_t pageEnd = Math::Min(lastDraw, pageStart + maxInstances);
                IndirectMeshGroup& group = m_groups.emplace_back();
                group.m_pipeline = first.m_pipeline;
                group.m_meshInstanceTable = first.m_meshInstanceTable;
                group.m_meshGroupTable = first.m_meshGroupTable;
                group.m_meshLodInfoTable = first.m_meshLodInfoTable;
                group.m_materialInstanceTable = first.m_materialInstanceTable;
                group.m_firstInstance = instanceIndices.size();
                group.m_instanceCount = pageEnd - pageStart;
                group.m_meshletCount = first.m_meshletCount;
                group.m_meshletX = meshletX;
                group.m_argumentIndex = groupCounts.size();

                for (uint32_t drawIndex = pageStart; drawIndex < pageEnd; ++drawIndex)
                    instanceIndices.push_back(draws[drawIndex]->m_instanceIndex);

                groupCounts.push_back({ meshletX, group.m_instanceCount, meshletZ });
            }

            firstDraw = lastDraw;
        }

        m_instanceIndices =
            Core::Buffer::CreateStructured<uint32_t>(graph.GetDevice(), "RenderInstanceIndices", instanceIndices.size());

        const auto counts = Core::Buffer::CreateStructured<MeshPass::MeshDispatchArguments>(graph.GetDevice(),
                                                                                            "RenderGroupCounts",
                                                                                            groupCounts.size());
        m_arguments = Core::Buffer::CreateStructured<MeshPass::MeshDispatchArguments>(graph.GetDevice(),
                                                                                      "RenderIndirectArgs",
                                                                                      groupCounts.size());

        FE_Verify(uploader.Upload(graph, //
                                  m_instanceIndices.Get(),
                                  instanceIndices.data(),
                                  instanceIndices.size() * sizeof(uint32_t)));
        FE_Verify(uploader.Upload(graph, //
                                  counts.Get(),
                                  groupCounts.data(),
                                  groupCounts.size() * sizeof(MeshPass::MeshDispatchArguments)));

        auto* passDesc = graph.AllocatePassData<MeshPass::BuildIndirectArgs::PassDesc>();
        passDesc->m_constants.m_groupCounts = graph.GetDescriptor(counts.Get());
        passDesc->m_constants.m_arguments = graph.GetDescriptor(m_arguments.Get());
        passDesc->m_constants.m_groupCount = groupCounts.size();
        passDesc->m_pipeline = IndirectArgsPipeline::GetPipeline();
        graph.AddDispatchPass("BuildIndirectArgs", passDesc, Math::CeilDivide(uint32_t(groupCounts.size()), 64u));
    }
} // namespace FE::Graphics
