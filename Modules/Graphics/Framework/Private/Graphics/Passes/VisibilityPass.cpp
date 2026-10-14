#include <Graphics/Core/PipelineVariantSet.h>
#include <Graphics/Passes/DrawTags.h>
#include <Graphics/Passes/RendererPassCommon.h>
#include <Graphics/Passes/VisibilityDebug.h>
#include <Graphics/Passes/VisibilityPass.h>
#include <Graphics/Scene/GpuMeshWorkBuilder.h>
#include <Shaders/Passes/Visibility/Debug.h>

namespace FE::Graphics::Visibility
{
    namespace
    {
        FE_COMPUTE_PIPELINE_SET(DebugPipeline, "Shaders/Passes/Visibility/Debug.cs.hlsl");
    } // namespace


    DebugMode ParseDebugMode(const festd::string_view value)
    {
        constexpr festd::string_view names[] = { "forward", "visibility", "instance", "meshlet",     "triangle",
                                                 "uv",      "normal",     "tangent",  "derivatives", "depth-error" };
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
        auto* debug = graph.AllocatePassData<DebugPassDesc>();
        debug->m_pipeline = DebugPipeline::GetPipeline();
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
