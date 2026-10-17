#include <Graphics/Passes/DrawTags.h>
#include <Graphics/Passes/RendererPassCommon.h>
#include <Graphics/Passes/VisibilityPass.h>
#include <Graphics/Scene/SurfaceEvaluation.h>
#include <Shaders/Passes/Visibility/Debug.h>

namespace FE::Graphics::Visibility
{
    ViewModule::ViewModule(View* view)
        : ViewModuleBase(view)
        , m_drawTag(DrawTags::Visibility)
    {
    }


    ViewModule::~ViewModule() = default;


    void ViewModule::DoRelease()
    {
        Memory::DefaultDelete(this);
    }


    void ViewModule::Update(Core::FrameGraphBlackboard& blackboard)
    {
        auto& data = blackboard.Add<PassData>();
        data.m_drawTag = m_drawTag;
        data.m_techniqueRole = m_techniqueRole;
    }


    void AddPasses(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard)
    {
        const PassData* data = blackboard.TryGet<PassData>();
        if (data == nullptr)
            return;

        FE_FG_SCOPE(graph, "DepthVisibility");

        const RendererViewData& view = blackboard.Get<RendererViewData>();
        const Vector2UInt resolution = view.m_mainColorTarget->GetDesc().GetSize2D();

        auto& output = blackboard.Add<Output>();
        output.m_target = Core::Texture::Create(graph.GetDevice(), "Visibility", Core::Format::kR32G32_UINT, resolution);

        auto* coverage = graph.AllocatePassData<CoveragePassDesc>();
        coverage->m_visibility = Core::TextureView::Create(output.m_target.Get());
        coverage->m_depth = Core::TextureView::Create(view.m_mainDepthTarget);
        coverage->m_viewport = view.m_viewportRect;
        graph.AddPass("ClearVisibility", coverage, [](Core::FrameGraphContext& context) {
            context.ClearColorTarget(0, Core::ColorClearValue::Uint(0));
            context.ClearDepthStencilTarget(0.0f, 0);
            context.ClearRenderTargets();
        });

        // All opaque object modules contribute coverage against the same depth and visibility targets.
        SceneRenderPass pass;
        pass.m_viewProjection = view.m_view->GetViewProjectionMatrix();
        pass.m_passDescToken = graph.AddBasePassDesc(coverage);
        pass.m_drawTag = data->m_drawTag;
        pass.m_techniqueRole = data->m_techniqueRole;
        view.m_scene->GetModules().ForEachActive([&](SceneModuleBase& module) {
            module.AddRenderPasses(graph, *view.m_renderQueueUploader, pass);
        });

        if (!IsReconstructionDebug(view.m_view->GetVisibilityDebugMode()))
            return;

        InitializeDebugColor(graph, view.m_mainColorTarget);

        // Reconstruction belongs to the object producer, which supplies its debug shader and data.
        for (const SurfaceEvaluationBatch& batch : blackboard.Get<SurfaceEvaluationInputs>())
        {
            auto* debug = graph.AllocatePassData<DebugPassDesc>();
            debug->m_pipeline = batch.m_visibilityDebugPipeline;
            debug->m_constants.m_view = graph.GetSRV(batch.m_objectData.Get());
            debug->m_constants.m_visibility = graph.GetSRV(output.m_target.Get());
            debug->m_constants.m_depth = graph.GetSRV(view.m_mainDepthTarget);
            debug->m_constants.m_output = graph.GetUAV(view.m_mainColorTarget);
            debug->m_constants.m_resolution = resolution;
            debug->m_constants.m_mode = festd::to_underlying(view.m_view->GetVisibilityDebugMode());
            graph.AddDispatchPass("VisibilityReconstruction", debug, Math::CeilDivide(resolution, 8u));
        }
    }
} // namespace FE::Graphics::Visibility
