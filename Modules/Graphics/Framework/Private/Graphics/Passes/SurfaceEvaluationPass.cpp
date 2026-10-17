#include <Graphics/Core/PipelineVariantSet.h>
#include <Graphics/Passes/RendererPassCommon.h>
#include <Graphics/Passes/SurfaceEvaluationPass.h>
#include <Graphics/Passes/VisibilityPass.h>
#include <Graphics/Scene/SurfaceEvaluation.h>
#include <Shaders/Deferred/Surface.h>
#include <Shaders/Passes/Visibility/Debug.h>

namespace FE::Graphics::SurfaceEvaluation
{
    using namespace Visibility;

    namespace
    {
        FE_COMPUTE_PIPELINE_SET(ClearSurfacePipeline, "Shaders/Deferred/ClearSurface.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(SurfaceDebugPipeline, "Shaders/Deferred/SurfaceDebug.cs.hlsl");


        void AddDebugPass(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard)
        {
            const RendererViewData& view = blackboard.Get<RendererViewData>();
            const auto& targets = blackboard.Get<Output>().m_gBufferTargets;
            const auto& visibilityData = blackboard.Get<Visibility::Output>();
            Core::Texture* visibility = visibilityData.m_target.Get();
            const Vector2UInt resolution = visibility->GetDesc().GetSize2D();
            if (view.m_view->GetVisibilityDebugMode() == DebugMode::kDeferred)
                return;

            auto* debug = graph.AllocatePassData<DebugPassDesc>();
            debug->m_pipeline = SurfaceDebugPipeline::GetPipeline();
            debug->m_constants.m_g0 = graph.GetSRV(targets[0].Get());
            debug->m_constants.m_g1 = graph.GetSRV(targets[1].Get());
            debug->m_constants.m_g2 = graph.GetSRV(targets[2].Get());
            debug->m_constants.m_g3 = graph.GetSRV(targets[3].Get());
            debug->m_constants.m_visibility = graph.GetSRV(visibility);
            debug->m_constants.m_depth = graph.GetSRV(view.m_mainDepthTarget);
            debug->m_constants.m_output = graph.GetUAV(view.m_mainColorTarget);
            debug->m_constants.m_resolution = resolution;
            debug->m_constants.m_mode = festd::to_underlying(view.m_view->GetVisibilityDebugMode());

            if (view.m_view->GetVisibilityDebugMode() == DebugMode::kEvaluationGroup)
            {
                for (const SurfaceEvaluationBatch& batch : blackboard.Get<SurfaceEvaluationInputs>())
                {
                    auto* groupDebug = graph.AllocatePassData<DebugPassDesc>();
                    *groupDebug = *debug;
                    groupDebug->m_pipeline = batch.m_surfaceDebugPipeline;
                    groupDebug->m_constants.m_view = graph.GetSRV(batch.m_objectData.Get());
                    graph.AddDispatchPass("SurfaceGroupDebug", groupDebug, Math::CeilDivide(resolution, 8u));
                }
            }
            else
            {
                graph.AddDispatchPass("GBufferDebug", debug, Math::CeilDivide(resolution, 8u));
            }
        }
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
        if (!blackboard.Contains<ViewModule*>() || !blackboard.Contains<Visibility::Output>())
            return;

        const RendererViewData& view = blackboard.Get<RendererViewData>();
        if (view.m_view->GetVisibilityDebugMode() < DebugMode::kEvaluationTile)
            return;

        FE_FG_SCOPE(graph, "SurfaceEvaluation");
        const Visibility::Output& visibilityData = blackboard.Get<Visibility::Output>();
        Core::Texture* visibility = visibilityData.m_target.Get();
        const Vector2UInt resolution = visibility->GetDesc().GetSize2D();

        auto& output = blackboard.Add<Output>();
        for (uint32_t targetIndex = 0; targetIndex < 4; ++targetIndex)
        {
            output.m_gBufferTargets[targetIndex] = Core::Texture::Create(graph.GetDevice(),
                                                                         Fmt::FormatName("GBuffer{}", targetIndex),
                                                                         Core::Format::kR32_UINT,
                                                                         resolution);
        }

        auto& targets = output.m_gBufferTargets;
        // Clear coverage once, then let each object producer classify only its own visibility IDs.
        auto* clear = graph.AllocatePassData<SurfaceDirectPassDesc>();
        clear->m_pipeline = ClearSurfacePipeline::GetPipeline();
        clear->m_constants.m_g0 = graph.GetUAV(targets[0].Get());
        clear->m_constants.m_resolution = resolution;
        clear->m_constants.m_visibility = graph.GetSRV(visibility);
        graph.AddDispatchPass("ClearSurfaceG0", clear, Math::CeilDivide(resolution, 8u));

        const auto pipelines = blackboard.Get<SceneData*>()->m_evaluationPipelines.GetPipelines();
        for (const SurfaceEvaluationBatch& batch : blackboard.Get<SurfaceEvaluationInputs>())
        {
            if (batch.m_routing->m_bindingCount == 0)
                continue;

            auto work = CreateSurfaceWork(graph.GetDevice(), resolution, *batch.m_routing, batch.m_waveLaneCount);
            {
                FE_FG_SCOPE(graph, "SurfaceClassification");
                BuildSurfaceWork(graph,
                                 *view.m_renderQueueUploader,
                                 batch.m_objectData.Get(),
                                 visibility,
                                 targets[0].Get(),
                                 work,
                                 false,
                                 batch.m_countPipeline,
                                 batch.m_scatterPipeline);
            }

            // Dispatch only shaders used by this producer, with its opaque reconstruction data.
            const uint32_t tilesPerRow = Math::CeilDivide(resolution.x, kTileSize);
            for (uint32_t slot = 0; slot < pipelines.size(); ++slot)
            {
                if (batch.m_routing->m_shaderBindingData[slot].y == 0)
                    continue;

                auto* passDesc = graph.AllocatePassData<SurfacePassDesc>();
                passDesc->m_pipeline = pipelines[slot].Get();
                passDesc->m_arguments =
                    Core::BufferView(work.m_arguments.Get()).Slice(slot * sizeof(DispatchArguments), sizeof(DispatchArguments));
                passDesc->m_constants.m_view = graph.GetSRV(batch.m_objectData.Get());
                passDesc->m_constants.m_visibility = graph.GetSRV(visibility);
                passDesc->m_constants.m_pixels = graph.GetSRV(work.m_pixels.Get());
                passDesc->m_constants.m_waves = graph.GetSRV(work.m_waves.Get());
                passDesc->m_constants.m_ranges = graph.GetSRV(work.m_ranges.Get());
                passDesc->m_constants.m_g0 = graph.GetUAV(targets[0].Get());
                passDesc->m_constants.m_g1 = graph.GetUAV(targets[1].Get());
                passDesc->m_constants.m_g2 = graph.GetUAV(targets[2].Get());
                passDesc->m_constants.m_g3 = graph.GetUAV(targets[3].Get());
                passDesc->m_constants.m_resolution = resolution;
                passDesc->m_constants.m_tilesPerRow = tilesPerRow;
                passDesc->m_constants.m_shaderSlot = slot;
                graph.AddDispatchIndirectPass("EvaluateSurface", passDesc);
            }
        }

        AddDebugPass(graph, blackboard);
    }
} // namespace FE::Graphics::SurfaceEvaluation
