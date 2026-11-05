#include <Graphics/Core/PipelineVariantSet.h>
#include <Graphics/Passes/DepthPyramidPass.h>
#include <Graphics/Passes/DrawTags.h>
#include <Graphics/Passes/ForwardTransparencyPass.h>
#include <Graphics/Passes/RendererPassCommon.h>
#include <Shaders/Lighting/ForwardTransparency.h>

namespace FE::Graphics::ForwardTransparency
{
    namespace
    {
        FE_COMPUTE_PIPELINE_SET(ResolveTransparencyPipeline, "Shaders/Lighting/ResolveTransparency.cs.hlsl");
    } // namespace


    ViewModule::ViewModule(View* view)
        : ViewModuleBase(view)
    {
    }


    void ViewModule::Update(Core::FrameGraphBlackboard& blackboard)
    {
        blackboard.Add<ViewModule*>(this);
    }


    void ViewModule::DoRelease()
    {
        Memory::DefaultDelete(this);
    }


    void AddPasses(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard)
    {
        if (!blackboard.Contains<ViewModule*>() || blackboard.Contains<ViewColorOverride>())
            return;

        FE_FG_SCOPE(graph, "ForwardTransparency");
        auto& view = blackboard.Get<RendererViewData>();
        if (!view.m_isMainColorWritten)
            return;

        const Vector2UInt resolution = view.m_mainColorTarget->GetDesc().GetSize2D();
        auto accumulation =
            Core::Texture::Create(graph.GetDevice(), "TransparencyAccumulation", Core::Format::kR16G16B16A16_SFLOAT, resolution);
        auto revealage = Core::Texture::Create(graph.GetDevice(), "TransparencyRevealage", Core::Format::kR16_SFLOAT, resolution);
        auto* targets = graph.AllocatePassData<Targets>();
        targets->m_accumulation = Core::TextureView::Create(accumulation.Get());
        targets->m_revealage = Core::TextureView::Create(revealage.Get());
        targets->m_depth = Core::TextureView::Create(view.m_mainDepthTarget);
        targets->m_viewport = view.m_viewportRect;
        graph.AddPass("ClearTransparency", targets, [](Core::FrameGraphContext& context) {
            context.ClearColorTarget(0, Core::ColorClearValue::Float(Color4F(0, 0, 0, 0)));
            context.ClearColorTarget(1, Core::ColorClearValue::Float(Color4F(1, 1, 1, 1)));
            context.ClearRenderTargets();
        });

        SceneRenderPass pass;
        pass.m_viewProjection = view.m_view->GetViewProjectionMatrix();
        pass.m_passDescToken = graph.AddBasePassDesc(targets);
        pass.m_drawTag = DrawTags::Transparent;
        pass.m_techniqueRole = "ForwardTransparency";
        pass.m_passData = view.m_lighting;
        pass.m_independentCulling = true;
        if (const auto* pyramids = blackboard.TryGet<DepthPyramidPass::Output>())
            pass.m_depthPyramid = &pyramids->m_farthest;

        view.m_scene->GetModules().ForEachActive([&](SceneModuleBase& module) {
            module.AddRenderPasses(graph, blackboard, *view.m_renderQueueUploader, pass);
        });

        TransparencyResolveConstants resolve{};
        resolve.m_accumulation = graph.GetSRV(accumulation.Get());
        resolve.m_revealage = graph.GetSRV(revealage.Get());
        resolve.m_color = graph.GetUAV(view.m_mainColorTarget);
        resolve.m_resolution = resolution;
        graph.AddDispatch("ResolveTransparency",
                          ResolveTransparencyPipeline::GetPipeline(),
                          resolve,
                          Math::CeilDivide(resolution, 8u));
    }
} // namespace FE::Graphics::ForwardTransparency
