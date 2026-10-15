#include <Graphics/Core/PipelineVariantSet.h>
#include <Graphics/Features/Mesh/MeshSceneModule.h>
#include <Graphics/Passes/DeferredShadingPass.h>
#include <Graphics/Passes/RendererPassCommon.h>
#include <Graphics/Passes/SurfaceEvaluationPass.h>
#include <Graphics/Passes/VisibilityPass.h>
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
            debug->m_constants.m_view = graph.GetSRV(visibilityData.m_view.Get());
            debug->m_constants.m_visibility = graph.GetSRV(visibility);
            debug->m_constants.m_depth = graph.GetSRV(view.m_mainDepthTarget);
            debug->m_constants.m_output = graph.GetUAV(view.m_mainColorTarget);
            debug->m_constants.m_resolution = resolution;
            debug->m_constants.m_mode = festd::to_underlying(view.m_view->GetVisibilityDebugMode());
            graph.AddDispatchPass("GBufferDebug", debug, Math::CeilDivide(resolution, 8u));
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
        blackboard.Add<PassData>().m_work = &m_work;
    }


    void AddPasses(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard)
    {
        if (!blackboard.Contains<PassData>() || !blackboard.Contains<Visibility::Output>())
            return;

        const RendererViewData& view = blackboard.Get<RendererViewData>();
        if (view.m_view->GetVisibilityDebugMode() < DebugMode::kEvaluationTile)
            return;

        FE_FG_SCOPE(graph, "SurfaceEvaluation");
        const PassData& data = blackboard.Get<PassData>();
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
        if (!view.m_scene->GetModules().Contains<MeshSceneModule>())
        {
            auto* clear = graph.AllocatePassData<SurfaceDirectPassDesc>();
            clear->m_pipeline = ClearSurfacePipeline::GetPipeline();
            clear->m_constants.m_g0 = graph.GetUAV(Core::TextureView::Create(targets[0].Get()));
            clear->m_constants.m_resolution = resolution;
            graph.AddDispatchPass("ClearSurfaceG0", clear, Math::CeilDivide(resolution, 8u));
            AddDebugPass(graph, blackboard);
            return;
        }

        const MeshSceneModule& meshes = view.m_scene->GetModules().Find<MeshSceneModule>();
        const auto pipelines = meshes.GetEvaluationPipelines().GetPipelines();
        festd::vector<SurfaceBinding> bindings;
        for (const MeshGroup* group : meshes.GetGroups())
        {
            if (group == nullptr || group->m_buffer == nullptr || group->m_instanceCount == 0)
                continue;

            const auto material = meshes.GetMaterialInstanceTable()->ReadRow(group->m_materialRef);
            const uint32_t slot = material.m_evaluationSlot.Get();
            if (slot < pipelines.size() && material.m_shadingSlot.Get() < 254)
                bindings.push_back({ group->m_tableRef.m_rowIndex, slot });
        }

        SurfaceWork& work = *data.m_work;
        const uint64_t previousScratchBytes = work.m_scratchBytes;
        const Vector2UInt previousResolution = work.m_resolution;
        EnsureSurfaceWork(work,
                          graph.GetDevice(),
                          resolution,
                          bindings,
                          meshes.GetMeshGroupTable()->GetReservedRowCount(),
                          pipelines.size(),
                          meshes.GetEvaluationWaveLaneCount());

        if (const auto* shading = blackboard.TryGet<DeferredShading::PassData>())
        {
            const auto shadingPipelines = meshes.GetShadingPipelines().GetPipelines();
            festd::vector<SurfaceBinding> shadingBindings;
            for (uint32_t slot = 0; slot < shadingPipelines.size(); ++slot)
            {
                if (shadingPipelines[slot] != nullptr)
                    shadingBindings.push_back({ slot, slot });
            }

            EnsureSurfaceWork(*shading->m_work,
                              graph.GetDevice(),
                              resolution,
                              shadingBindings,
                              shadingPipelines.size(),
                              shadingPipelines.size(),
                              32);
            ShareSurfaceStorage(graph.GetDevice(), work, *shading->m_work);
        }

        if (previousScratchBytes != work.m_scratchBytes || previousResolution != resolution)
        {
            uint64_t scratchBytes = work.m_scratchBytes;
            if (const auto* shading = blackboard.TryGet<DeferredShading::PassData>())
            {
                scratchBytes += shading->m_work->m_scratchBytes;
                scratchBytes -= work.m_pixels->GetDesc().m_size;
                scratchBytes -= work.m_waves->GetDesc().m_size;
            }

            const uint64_t pixels = uint64_t(resolution.x) * resolution.y;
            Logger::LogInfo(
                "Deferred view {}x{}: {} bytes of visibility/GBuffer images, {} bytes of shared HDR/depth, {} bytes of classification scratch",
                resolution.x,
                resolution.y,
                pixels * 24,
                pixels * 8,
                scratchBytes);
        }

        {
            FE_FG_SCOPE(graph, "SurfaceClassification");
            BuildSurfaceWork(graph, *view.m_renderQueueUploader, visibilityData.m_view.Get(), visibility, targets[0].Get(), work);
        }

        const uint32_t tilesPerRow = Math::CeilDivide(resolution.x, kTileSize);
        for (uint32_t slot = 0; slot < pipelines.size(); ++slot)
        {
            if (pipelines[slot] == nullptr)
                continue;

            auto* passDesc = graph.AllocatePassData<SurfacePassDesc>();
            passDesc->m_pipeline = pipelines[slot].Get();
            passDesc->m_arguments =
                Core::BufferView(work.m_arguments.Get()).Slice(slot * sizeof(DispatchArguments), sizeof(DispatchArguments));
            passDesc->m_constants.m_view = graph.GetSRV(visibilityData.m_view.Get());
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
        AddDebugPass(graph, blackboard);
    }
} // namespace FE::Graphics::SurfaceEvaluation
