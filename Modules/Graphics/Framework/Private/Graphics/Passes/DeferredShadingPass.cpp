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
        const SurfaceEvaluation::Output* surfaces = blackboard.TryGet<SurfaceEvaluation::Output>();
        if (!blackboard.Contains<ViewModule*>() || surfaces == nullptr || blackboard.Contains<ViewColorOverride>())
            return;

        RendererViewData& view = blackboard.Get<RendererViewData>();
        const auto& gBufferTargets = surfaces->m_gBufferTargets;

        FE_FG_SCOPE(graph, "DeferredShading");
        const Vector2UInt resolution = gBufferTargets[0]->GetDesc().GetSize2D();
        const auto pipelines = blackboard.Get<SceneData*>()->m_shadingPipelines.GetSlots();
        const uint32_t shaderCount = pipelines.size();
        // Initialize background and unsupported routes before shading only covered pixels.
        DeferredConstants constants{};
        constants.m_view = view.m_constants;
        constants.m_lighting = view.m_lighting;
        constants.m_g0 = graph.GetSRV(Core::TextureView::Create(gBufferTargets[0].Get()));
        constants.m_output = graph.GetUAV(Core::TextureView::Create(view.m_mainColorTarget));
        constants.m_work.m_resolution = resolution;
        constants.m_shaderCount = shaderCount;
        graph.AddDispatch("InitializeDeferredColor",
                          InitializeDeferredPipeline::GetPipeline(),
                          constants,
                          Math::CeilDivide(resolution, 8u));
        view.m_isMainColorWritten = true;
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

        constants.m_g1 = graph.GetSRV(gBufferTargets[1].Get());
        constants.m_g2 = graph.GetSRV(gBufferTargets[2].Get());
        constants.m_g3 = graph.GetSRV(gBufferTargets[3].Get());
        constants.m_depth = graph.GetSRV(view.m_mainDepthTarget);
        constants.m_work.m_pixels = graph.GetSRV(work.m_pixels.Get());
        constants.m_work.m_waves = graph.GetSRV(work.m_waves.Get());
        constants.m_work.m_ranges = graph.GetSRV(work.m_ranges.Get());
        constants.m_work.m_tilesPerRow = Math::CeilDivide(resolution.x, kTileSize);
        for (uint32_t slot = 0; slot < pipelines.size(); ++slot)
        {
            if (pipelines[slot].m_pipeline == nullptr)
                continue;

            constants.m_work.m_shaderSlot = slot;
            const Core::BufferView arguments =
                Core::BufferView(work.m_arguments.Get()).Slice(slot * sizeof(DispatchArguments), sizeof(DispatchArguments));
            graph.AddDispatchIndirect("ShadeSurface", pipelines[slot].m_pipeline.Get(), constants, arguments);
        }
    }
} // namespace FE::Graphics::DeferredShading
