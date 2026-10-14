#include <Graphics/Core/PipelineVariantSet.h>
#include <Graphics/Features/Mesh/MeshSceneModule.h>
#include <Graphics/Passes/DrawTags.h>
#include <Graphics/Passes/RendererPassCommon.h>
#include <Graphics/Passes/SurfaceWorkBuilder.h>
#include <Graphics/Passes/VisibilityDebug.h>
#include <Graphics/Passes/VisibilityPass.h>
#include <Graphics/Scene/GpuMeshWorkBuilder.h>
#include <Shaders/Passes/Visibility/Debug.h>
#include <Shaders/Passes/Visibility/Surface.h>

namespace FE::Graphics::Visibility
{
    namespace
    {
        FE_COMPUTE_PIPELINE_SET(SurfaceDebugPipeline, "Shaders/Passes/Visibility/SurfaceDebug.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(ClearSurfacePipeline, "Shaders/Passes/Visibility/ClearSurface.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(DebugPipeline, "Shaders/Passes/Visibility/Debug.cs.hlsl");


        void EvaluateSurfaces(Core::FrameGraph& graph, const RendererViewData& view, Core::Buffer* viewBuffer,
                              Core::Texture* visibility, Rc<Core::Texture> (&planes)[4])
        {
            const Vector2UInt resolution(visibility->GetDesc().m_width, visibility->GetDesc().m_height);
            for (uint32_t plane = 0; plane < 4; ++plane)
                planes[plane] = Core::Texture::Create(graph.GetDevice(),
                                                      Fmt::FormatName("SurfaceG{}", plane),
                                                      Core::Format::kR32_UINT,
                                                      resolution);

            auto* clear = graph.AllocatePassData<SurfaceDirectPassDesc>();
            clear->m_pipeline = ClearSurfacePipeline::GetPipeline();
            clear->m_constants.m_g0 = graph.GetUAV(Core::TextureView::Create(planes[0].Get()));
            clear->m_constants.m_resolution = resolution;
            graph.AddDispatchPass(
                "ClearSurfaceG0",
                clear,
                Core::ComputeWorkGroupCount(Vector2UInt(Math::CeilDivide(resolution.x, 8u), Math::CeilDivide(resolution.y, 8u))));
            if (!view.m_scene->GetModules().Contains<MeshSceneModule>())
                return;

            const MeshSceneModule& meshes = view.m_scene->GetModules().Find<MeshSceneModule>();
            const auto pipelines = meshes.GetEvaluationPipelines().GetPipelines();
            const SurfaceWork work = CreateSurfaceWork(graph.GetDevice(), pipelines.size());
            const uint32_t tilesPerRow = Math::CeilDivide(resolution.x, kTileSize);
            const uint32_t tiles = tilesPerRow * Math::CeilDivide(resolution.y, kTileSize);
            for (uint32_t firstTile = 0; firstTile < tiles; firstTile += work.m_tileCapacity)
            {
                FE_FG_SCOPE(graph, Fmt::FixedFormat("SurfaceTiles{}", firstTile));
                BuildSurfaceWork(graph,
                                 viewBuffer,
                                 visibility,
                                 planes[0].Get(),
                                 work,
                                 firstTile,
                                 Math::Min(work.m_tileCapacity, tiles - firstTile),
                                 meshes.GetEvaluationWaveLaneCount());
                for (uint32_t slot = 0; slot < pipelines.size(); ++slot)
                {
                    auto* evaluation = graph.AllocatePassData<SurfacePassDesc>();
                    evaluation->m_pipeline = pipelines[slot].Get();
                    evaluation->m_arguments = Core::BufferView(work.m_arguments.Get())
                                                  .Slice(slot * sizeof(DispatchArguments), sizeof(DispatchArguments));
                    evaluation->m_constants.m_view = graph.GetSRV(viewBuffer);
                    evaluation->m_constants.m_visibility = graph.GetSRV(Core::TextureView::Create(visibility));
                    evaluation->m_constants.m_pixels = graph.GetSRV(work.m_pixels.Get());
                    evaluation->m_constants.m_waves = graph.GetSRV(work.m_waves.Get());
                    evaluation->m_constants.m_ranges = graph.GetSRV(work.m_ranges.Get());
                    evaluation->m_constants.m_g0 = graph.GetUAV(Core::TextureView::Create(planes[0].Get()));
                    evaluation->m_constants.m_g1 = graph.GetUAV(Core::TextureView::Create(planes[1].Get()));
                    evaluation->m_constants.m_g2 = graph.GetUAV(Core::TextureView::Create(planes[2].Get()));
                    evaluation->m_constants.m_g3 = graph.GetUAV(Core::TextureView::Create(planes[3].Get()));
                    evaluation->m_constants.m_resolution = resolution;
                    evaluation->m_constants.m_tilesPerRow = tilesPerRow;
                    evaluation->m_constants.m_shaderSlot = slot;
                    graph.AddPass("EvaluateSurface", evaluation, [](Core::FrameGraphContext& context) {
                        context.DispatchIndirect();
                    });
                }
            }
        }
    } // namespace


    DebugMode ParseDebugMode(const festd::string_view value)
    {
        constexpr festd::string_view names[] = {
            "forward",       "visibility",     "instance",           "meshlet",          "triangle",        "uv",
            "normal",        "tangent",        "derivatives",        "depth-error",      "evaluation-tile", "evaluation-group",
            "surface-color", "surface-normal", "surface-properties", "surface-emissive", "surface-route"
        };
        for (uint32_t index = 0; index < festd::size(names); ++index)
        {
            if (value == names[index])
                return static_cast<DebugMode>(index);
        }

        FE_Assert(false, "Unknown visibility debug mode");
        return DebugMode::kForward;
    }


    void AddPasses(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard, Core::Texture* visibility)
    {
        const RendererViewData& view = blackboard.Get<RendererViewData>();
        auto* coverage = graph.AllocatePassData<CoveragePassDesc>();
        coverage->m_visibility = Core::TextureView::Create(visibility);
        coverage->m_depth = Core::TextureView::Create(view.m_mainDepthTarget);
        coverage->m_viewport = view.m_viewportRect;
        graph.AddPass("ClearVisibility", coverage, [](Core::FrameGraphContext& context) {
            context.ClearColorTarget(0, Core::ColorClearValue::Uint(0));
            context.ClearDepthStencilTarget(0.0f, 0);
            context.ClearRenderTargets();
        });

        SceneRenderPass pass;
        pass.m_viewProjection = view.m_view->GetViewProjectionMatrix();
        pass.m_passDescToken = graph.AddBasePassDesc(coverage);
        pass.m_drawTag = DrawTags::Opaque;
        pass.m_techniqueRole = "DepthVisibility";
        view.m_scene->GetModules().ForEachActive([&](SceneModuleBase& module) {
            module.AddRenderPasses(graph, *view.m_renderQueueUploader, pass);
        });

        MeshPass::ViewData geometryView{};
        if (blackboard.Contains<CulledMeshView>())
            geometryView = blackboard.Get<CulledMeshView>().m_viewData;

        Rc<Core::Buffer> viewBuffer = Core::Buffer::CreateStructured<MeshPass::ViewData>(graph.GetDevice(), "VisibilityView", 1);
        FE_Verify(view.m_renderQueueUploader->UploadArray(graph, viewBuffer.Get(), festd::span(&geometryView, 1)));
        const bool surfaceDebug = view.m_view->GetVisibilityDebugMode() >= DebugMode::kEvaluationTile;
        Rc<Core::Texture> planes[4];
        if (surfaceDebug)
            EvaluateSurfaces(graph, view, viewBuffer.Get(), visibility, planes);

        auto* debug = graph.AllocatePassData<DebugPassDesc>();
        debug->m_pipeline = surfaceDebug ? SurfaceDebugPipeline::GetPipeline() : DebugPipeline::GetPipeline();
        if (surfaceDebug)
        {
            debug->m_constants.m_g0 = graph.GetSRV(Core::TextureView::Create(planes[0].Get()));
            debug->m_constants.m_g1 = graph.GetSRV(Core::TextureView::Create(planes[1].Get()));
            debug->m_constants.m_g2 = graph.GetSRV(Core::TextureView::Create(planes[2].Get()));
            debug->m_constants.m_g3 = graph.GetSRV(Core::TextureView::Create(planes[3].Get()));
        }
        debug->m_constants.m_view = graph.GetSRV(viewBuffer.Get());
        debug->m_constants.m_visibility = graph.GetSRV(Core::TextureView::Create(visibility));
        debug->m_constants.m_depth = graph.GetSRV(Core::TextureView::Create(view.m_mainDepthTarget));
        debug->m_constants.m_output = graph.GetUAV(Core::TextureView::Create(view.m_mainColorTarget));
        debug->m_constants.m_resolution = { visibility->GetDesc().m_width, visibility->GetDesc().m_height };
        debug->m_constants.m_mode = static_cast<uint32_t>(view.m_view->GetVisibilityDebugMode());
        graph.AddDispatchPass("VisibilityReconstruction",
                              debug,
                              Core::ComputeWorkGroupCount(Vector2UInt(Math::CeilDivide(visibility->GetDesc().m_width, 8u),
                                                                      Math::CeilDivide(visibility->GetDesc().m_height, 8u))));
    }
} // namespace FE::Graphics::Visibility
