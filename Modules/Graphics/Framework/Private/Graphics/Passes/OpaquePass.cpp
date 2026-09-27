#include <Core/Math/Colors.h>
#include <Graphics/Passes/DepthPrepass.h>
#include <Graphics/Passes/DrawTags.h>
#include <Graphics/Passes/OpaquePass.h>
#include <Graphics/Passes/RendererPassCommon.h>
#include <Graphics/Scene/RenderBatch.h>
#include <Graphics/Tables/MaterialInstanceTable.h>
#include <Graphics/Tables/MeshGroupTable.h>
#include <Graphics/Tables/MeshInstanceTable.h>
#include <Graphics/Tables/MeshLodInfoTable.h>

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

        festd::inline_vector<const RenderDraw*, 64> draws;
        for (const RenderBatch& batch : batches.GetBatches())
        {
            for (const RenderDraw& draw : batch.m_draws)
                draws.push_back(&draw);
        }

        festd::sort(draws, [](const RenderDraw* lhs, const RenderDraw* rhs) {
            return reinterpret_cast<uintptr_t>(lhs->m_pipeline) < reinterpret_cast<uintptr_t>(rhs->m_pipeline);
        });

        bool isFirstDraw = true;
        for (const RenderDraw* draw : draws)
        {
            auto* passDesc = graph.AllocatePassData<PassDesc>();
            passDesc->m_constants.m_viewProjection = draw->m_viewProjection;
            passDesc->m_constants.m_meshInstanceTable = draw->m_meshInstanceTable;
            passDesc->m_constants.m_meshGroupTable = draw->m_meshGroupTable;
            passDesc->m_constants.m_meshLodInfoTable = draw->m_meshLodInfoTable;
            passDesc->m_constants.m_materialInstanceTable = draw->m_materialInstanceTable;
            passDesc->m_constants.m_instanceIndex = draw->m_instanceIndex;
            passDesc->m_colorTarget = Core::TextureView::Create(viewData.m_mainColorTarget);
            passDesc->m_depthTarget = Core::TextureView::Create(viewData.m_mainDepthTarget);
            passDesc->m_viewport = viewData.m_viewportRect;
            passDesc->m_pipeline = draw->m_pipeline;

            graph.AddPass("OpaquePass",
                          passDesc,
                          [meshletCount = draw->m_meshletCount, isFirstDraw, hasDepthPrepass](Core::FrameGraphContext& context) {
                              if (isFirstDraw)
                              {
                                  context.ClearColorTarget(0, Colors::kDarkSlateBlue);
                                  if (!hasDepthPrepass)
                                      context.ClearDepthStencilTarget(0.0f, 0);
                              }
                              context.DispatchMesh(meshletCount);
                          });
            isFirstDraw = false;
        }
    }
} // namespace FE::Graphics::OpaquePass
