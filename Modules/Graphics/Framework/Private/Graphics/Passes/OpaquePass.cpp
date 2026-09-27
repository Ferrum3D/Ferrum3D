#include <Core/Math/Colors.h>
#include <Graphics/Passes/DepthPrepass.h>
#include <Graphics/Passes/DrawTags.h>
#include <Graphics/Passes/OpaquePass.h>
#include <Graphics/Passes/RendererPassCommon.h>
#include <Graphics/Scene/IndirectMeshBatcher.h>
#include <Graphics/Scene/RenderBatch.h>
#include <Graphics/Tables/MaterialInstanceTable.h>
#include <Graphics/Tables/MeshGroupTable.h>
#include <Graphics/Tables/MeshInstanceTable.h>
#include <Graphics/Tables/MeshLodInfoTable.h>

#include <Shaders/Passes/MeshPass/IndirectArguments.h>
#include <Shaders/Passes/OpaquePass/OpaquePass.h>

namespace FE::Graphics::OpaquePass
{
    ViewModule::ViewModule(View* view)
        : ViewModuleBase(view)
    {
    }


    ViewModule::~ViewModule() = default;


    void ViewModule::DoRelease()
    {
        Memory::DefaultDelete(this);
    }


    void ViewModule::Update(Core::FrameGraphBlackboard& blackboard)
    {
        blackboard.Add<PassData>();
    }


    void AddPasses(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard)
    {
        if (!blackboard.Contains<PassData>())
            return;

        const bool hasDepthPrepass =
            blackboard.Contains<DepthPrepass::PassData>() && blackboard.Get<DepthPrepass::PassData>().m_hasDraws;

        const RendererViewData& viewData = blackboard.Get<RendererViewData>();
        RenderBatchCollector batches(graph.GetAllocator(),
                                     viewData.m_view->GetViewProjectionMatrix(),
                                     viewData.m_mainColorTarget->GetDesc().m_imageFormat,
                                     DrawTags::Opaque,
                                     "Opaque");
        batches.Collect(*viewData.m_scene);

        IndirectMeshBatcher batcher(graph.GetAllocator());
        batcher.Build(graph, *viewData.m_renderQueueUploader, batches.GetBatches());

        bool isFirstDraw = true;
        for (const IndirectMeshGroup& group : batcher.m_groups)
        {
            auto* passDesc = graph.AllocatePassData<PassDesc>();
            passDesc->m_constants.m_viewProjection = viewData.m_view->GetViewProjectionMatrix();
            passDesc->m_constants.m_meshInstanceTable = group.m_meshInstanceTable;
            passDesc->m_constants.m_meshGroupTable = group.m_meshGroupTable;
            passDesc->m_constants.m_meshLodInfoTable = group.m_meshLodInfoTable;
            passDesc->m_constants.m_materialInstanceTable = group.m_materialInstanceTable;
            passDesc->m_constants.m_instanceIndices = graph.GetDescriptor(batcher.m_instanceIndices.Get());
            passDesc->m_constants.m_firstInstance = group.m_firstInstance;
            passDesc->m_constants.m_meshletCount = group.m_meshletCount;
            passDesc->m_constants.m_meshletX = group.m_meshletX;
            passDesc->m_colorTarget = Core::TextureView::Create(viewData.m_mainColorTarget);
            passDesc->m_depthTarget = Core::TextureView::Create(viewData.m_mainDepthTarget);
            passDesc->m_viewport = viewData.m_viewportRect;
            passDesc->m_pipeline = group.m_pipeline;
            passDesc->m_arguments = { batcher.m_arguments.Get(),
                                      Core::BarrierSyncFlags::kExecuteIndirect,
                                      Core::BarrierAccessFlags::kIndirectArgument };

            graph.AddPass("OpaquePass",
                          passDesc,
                          [arguments = Core::BufferView(batcher.m_arguments.Get()),
                           argumentIndex = group.m_argumentIndex,
                           isFirstDraw,
                           hasDepthPrepass](Core::FrameGraphContext& context) {
                              if (isFirstDraw)
                              {
                                  context.ClearColorTarget(0, Colors::kDarkSlateBlue);
                                  if (!hasDepthPrepass)
                                      context.ClearDepthStencilTarget(0.0f, 0);
                              }
                              context.DispatchMeshIndirect(arguments, argumentIndex * sizeof(MeshPass::MeshDispatchArguments));
                          });
            isFirstDraw = false;
        }
    }
} // namespace FE::Graphics::OpaquePass
