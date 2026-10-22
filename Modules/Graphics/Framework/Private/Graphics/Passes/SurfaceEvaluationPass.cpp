#include <Graphics/Core/PipelineVariantSet.h>
#include <Graphics/Passes/RendererPassCommon.h>
#include <Graphics/Passes/SurfaceEvaluationPass.h>
#include <Graphics/Passes/VisibilityPass.h>
#include <Graphics/Scene/SurfaceEvaluation.h>
#include <Shaders/Deferred/Surface.h>

namespace FE::Graphics::SurfaceEvaluation
{
    using namespace Visibility;

    namespace
    {
        FE_COMPUTE_PIPELINE_SET(ClearSurfacePipeline, "Shaders/Deferred/ClearSurface.cs.hlsl");
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
        const Visibility::Output* visibilityData = blackboard.TryGet<Visibility::Output>();
        if (!blackboard.Contains<ViewModule*>() || visibilityData == nullptr)
            return;

        // Debug output that does not read the GBuffer makes evaluation redundant.
        if (const ViewColorOverride* colorOverride = blackboard.TryGet<ViewColorOverride>())
        {
            if (!colorOverride->m_needsSurfaceEvaluation)
                return;
        }

        FE_FG_SCOPE(graph, "SurfaceEvaluation");

        const RendererViewData& view = blackboard.Get<RendererViewData>();
        Core::Texture* visibility = visibilityData->m_target.Get();
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

        const auto pipelines = blackboard.Get<SceneData*>()->m_evaluationPipelines.GetSlots();
        for (const SurfaceEvaluationBatch& batch : blackboard.Get<SurfaceEvaluationInputs>())
        {
            if (batch.m_routing->m_bindingCount == 0)
                continue;

            auto work = CreateSurfaceWork(graph.GetDevice(), resolution, *batch.m_routing, batch.m_waveLaneCount);
            {
                FE_FG_SCOPE(graph, "SurfaceClassification");
                const EvaluationWorkInput input{ batch.m_objectData.Get(),
                                                 visibility,
                                                 batch.m_countPipeline,
                                                 batch.m_scatterPipeline };
                BuildSurfaceWork(graph, *view.m_renderQueueUploader, input, work);
            }

            // Dispatch only shaders used by this producer, with its opaque reconstruction data.
            const uint32_t tilesPerRow = Math::CeilDivide(resolution.x, kTileSize);
            for (uint32_t slot = 0; slot < pipelines.size(); ++slot)
            {
                if (batch.m_routing->m_shaderBindingData[slot].y == 0)
                    continue;

                auto* passDesc = graph.AllocatePassData<SurfacePassDesc>();
                passDesc->m_pipeline = pipelines[slot].m_pipeline.Get();
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
    }
} // namespace FE::Graphics::SurfaceEvaluation
