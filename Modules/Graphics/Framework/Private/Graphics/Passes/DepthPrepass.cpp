#include <FeCore/Math/Colors.h>
#include <Graphics/Features/Mesh/MeshSceneModule.h>
#include <Graphics/Passes/DepthPrepass.h>
#include <Graphics/Passes/DrawTags.h>
#include <Graphics/Passes/RendererPassCommon.h>
#include <Graphics/Tables/MeshGroupTable.h>
#include <Graphics/Tables/MeshInstanceTable.h>
#include <Graphics/Tables/MeshLodInfoTable.h>
#include <Graphics/Core/PipelineVariantSet.h>

#include <Shaders/Passes/DepthPrepass/DepthPrepass.h>

namespace FE::Graphics::DepthPrepass
{
    namespace
    {
        struct Pipeline final : public Core::GraphicsPipelineVariantSet
        {
            FE_DECLARE_PIPELINE_SET(Pipeline);

        private:
            void SetupRequest([[maybe_unused]] const uint32_t variantIndex, Core::GraphicsPipelineRequest& request) override
            {
                request.m_desc.SetMeshShader("Shaders/Passes/OpaquePass/OpaquePass.ms.hlsl")
                    .SetDSVFormat(Core::Format::kD32_SFLOAT_S8_UINT)
                    .SetDepthStencil(Core::DepthStencilState::kWriteIfGreater)
                    .SetRasterization(Core::RasterizationState::kFillBackCull);
            }
        };
        FE_IMPLEMENT_PIPELINE_SET(Pipeline);
    } // namespace


    ViewModule::ViewModule(View* view)
        : ViewModuleBase(view)
    {
    }


    ViewModule::~ViewModule() = default;


    void ViewModule::Update(Core::FrameGraphBlackboard& blackboard)
    {
        blackboard.Add<PassData>();
    }


    void AddPasses(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard, Scene& scene)
    {
        if (blackboard.TryGet<PassData>() == nullptr)
            return;

        const Internal::RendererViewData& viewData = blackboard.Get<Internal::RendererViewData>();
        auto* meshModule = scene.GetModules().TryFind<MeshSceneModule>();
        if (meshModule == nullptr)
            return;

        const DB::Ref<MeshInstanceTable> instanceRef{ 0 };
        MeshBatch* batch = meshModule->FindBatch(instanceRef);
        if (batch == nullptr || !batch->m_drawTagMask.Intersects(DrawTagMask(DrawTags::DepthPrepass)))
            return;

        const MeshInstanceTable::Row instanceRow = meshModule->GetMeshInstanceTable()->ReadRow(instanceRef);
        const MeshGroupTable::Row groupRow = meshModule->GetMeshGroupTable()->ReadRow(instanceRow.m_meshGroup.Get());
        const DB::Slice<MeshLodInfoTable> lods = groupRow.m_lods.Get();
        const Core::MeshLodInfo lodInfo = meshModule->GetMeshLodInfoTable()->ReadRow(lods.m_rowIndex).m_info.Get();

        auto* passDesc = graph.AllocatePassData<::FE::Graphics::DepthPrepass::PassDesc>();
        passDesc->m_constants.m_meshInstanceTable =
            viewData.m_database->GetTableBufferPointer(graph, *meshModule->GetMeshInstanceTable());
        passDesc->m_constants.m_meshGroupTable =
            viewData.m_database->GetTableBufferPointer(graph, *meshModule->GetMeshGroupTable());
        passDesc->m_constants.m_meshLodInfoTable =
            viewData.m_database->GetTableBufferPointer(graph, *meshModule->GetMeshLodInfoTable());
        passDesc->m_constants.m_instanceIndex = instanceRef.m_rowIndex;
        passDesc->m_constants.m_viewProjection = viewData.m_view->GetViewProjectionMatrix();
        passDesc->m_depthTarget = Core::TextureView::Create(viewData.m_mainDepthTarget);
        passDesc->m_viewport = viewData.m_viewportRect;
        passDesc->m_pipeline = Pipeline::GetPipeline();

        graph.AddPass("DepthPrepass", passDesc, [meshletCount = lodInfo.m_meshletCount](Core::FrameGraphContext& context) {
            context.SetRenderTargetLoadOperations(Core::RenderTargetLoadOperations{}.ClearDepthStencil(0.0f, 0));
            context.SetRenderTargetStoreOperations(Core::RenderTargetStoreOperations::kDefault);
            context.DispatchMesh(meshletCount);
        });
    }
} // namespace FE::Graphics::DepthPrepass
