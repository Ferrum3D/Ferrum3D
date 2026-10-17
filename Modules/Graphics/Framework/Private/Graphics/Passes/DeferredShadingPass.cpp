#include <Graphics/Core/PipelineVariantSet.h>
#include <Graphics/Passes/DeferredShadingPass.h>
#include <Graphics/Passes/RendererPassCommon.h>
#include <Graphics/Passes/SurfaceEvaluationPass.h>
#include <Graphics/Passes/SurfaceWorkBuilder.h>
#include <Shaders/Deferred/Deferred.h>

namespace FE::Graphics::DeferredShading
{
    using namespace Visibility;

    namespace
    {
        FE_COMPUTE_PIPELINE_SET(InitializeDeferredPipeline, "Shaders/Deferred/InitializeDeferred.cs.hlsl");
    } // namespace


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
        blackboard.Add<ViewModule*>(this);
    }


    void AddPasses(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard)
    {
        if (!blackboard.Contains<ViewModule*>() || !blackboard.Contains<SurfaceEvaluation::Output>())
            return;

        const RendererViewData& view = blackboard.Get<RendererViewData>();
        if (!NeedsDeferredShading(view.m_view->GetVisibilityDebugMode()))
            return;

        const auto& gBufferTargets = blackboard.Get<SurfaceEvaluation::Output>().m_gBufferTargets;

        FE_FG_SCOPE(graph, "DeferredShading");
        const Vector2UInt resolution = gBufferTargets[0]->GetDesc().GetSize2D();
        Core::Buffer* viewBuffer = view.m_constants.Get();
        const auto pipelines = blackboard.Get<SceneData*>()->m_shadingPipelines.GetSlots();
        const uint32_t shaderCount = pipelines.size();
        // Initialize background and unsupported routes before shading only covered pixels.
        auto* initialize = graph.AllocatePassData<DeferredDirectPassDesc>();
        initialize->m_pipeline = InitializeDeferredPipeline::GetPipeline();
        initialize->m_constants.m_view = graph.GetSRV(viewBuffer);
        initialize->m_constants.m_g0 = graph.GetSRV(Core::TextureView::Create(gBufferTargets[0].Get()));
        initialize->m_constants.m_output = graph.GetUAV(Core::TextureView::Create(view.m_mainColorTarget));
        initialize->m_constants.m_resolution = resolution;
        initialize->m_constants.m_shaderCount = shaderCount;
        graph.AddDispatchPass("InitializeDeferredColor", initialize, Math::CeilDivide(resolution, 8u));
        if (pipelines.empty())
            return;

        // Regroup evaluated pixels by shading shader; these routes are independent of evaluation bindings.
        SceneData& scene = *blackboard.Get<SceneData*>();
        if (scene.m_shadingRoutingRevision != scene.m_shadingPipelines.GetRevision())
        {
            festd::inline_vector<SurfaceBinding, 16> bindings;
            for (uint32_t slot = 0; slot < pipelines.size(); ++slot)
            {
                if (pipelines[slot].m_pipeline != nullptr)
                    bindings.push_back({ slot, slot });
            }

            scene.m_shadingRouting = CreateSurfaceRouting(graph.GetDevice(), bindings, shaderCount, shaderCount);
            scene.m_shadingRoutingRevision = scene.m_shadingPipelines.GetRevision();
        }

        auto work = CreateSurfaceWork(graph.GetDevice(), resolution, scene.m_shadingRouting, 32);
        {
            FE_FG_SCOPE(graph, "ShadingClassification");
            BuildSurfaceWork(graph, *view.m_renderQueueUploader, ShadingWorkInput{ gBufferTargets[0].Get() }, work);
        }

        for (uint32_t slot = 0; slot < pipelines.size(); ++slot)
        {
            if (pipelines[slot].m_pipeline == nullptr)
                continue;

            auto* shading = graph.AllocatePassData<DeferredPassDesc>();
            shading->m_pipeline = pipelines[slot].m_pipeline.Get();
            shading->m_arguments =
                Core::BufferView(work.m_arguments.Get()).Slice(slot * sizeof(DispatchArguments), sizeof(DispatchArguments));
            shading->m_constants.m_view = graph.GetSRV(viewBuffer);
            shading->m_constants.m_g0 = graph.GetSRV(gBufferTargets[0].Get());
            shading->m_constants.m_g1 = graph.GetSRV(gBufferTargets[1].Get());
            shading->m_constants.m_g2 = graph.GetSRV(gBufferTargets[2].Get());
            shading->m_constants.m_g3 = graph.GetSRV(gBufferTargets[3].Get());
            shading->m_constants.m_depth = graph.GetSRV(view.m_mainDepthTarget);
            shading->m_constants.m_output = graph.GetUAV(view.m_mainColorTarget);
            shading->m_constants.m_pixels = graph.GetSRV(work.m_pixels.Get());
            shading->m_constants.m_waves = graph.GetSRV(work.m_waves.Get());
            shading->m_constants.m_ranges = graph.GetSRV(work.m_ranges.Get());
            shading->m_constants.m_resolution = resolution;
            shading->m_constants.m_tilesPerRow = Math::CeilDivide(resolution.x, kTileSize);
            shading->m_constants.m_shaderSlot = slot;
            shading->m_constants.m_shaderCount = pipelines.size();
            graph.AddDispatchIndirectPass("ShadeSurface", shading);
        }
    }
} // namespace FE::Graphics::DeferredShading
