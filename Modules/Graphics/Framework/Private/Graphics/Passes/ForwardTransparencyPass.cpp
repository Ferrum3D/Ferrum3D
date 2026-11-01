#include <Graphics/Core/PipelineVariantSet.h>
#include <Graphics/Features/Shadows/SunShadow.h>
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
        const SunShadow::Output* sunShadow = blackboard.TryGet<SunShadow::Output>();
        if (sunShadow != nullptr)
        {
            targets->m_sunShadowAtlas = Core::PassTextureAccess(Core::TextureView::Create(sunShadow->m_atlas.Get()),
                                                                Core::BarrierSyncFlags::kPixelShading,
                                                                Core::BarrierAccessFlags::kShaderRead,
                                                                Core::BarrierLayout::kShaderRead);
        }

        graph.AddPass("ClearTransparency", targets, [](Core::FrameGraphContext& context) {
            context.ClearColorTarget(0, Core::ColorClearValue::Float(Color4F(0, 0, 0, 0)));
            context.ClearColorTarget(1, Core::ColorClearValue::Float(Color4F(1, 1, 1, 1)));
            context.ClearRenderTargets();
        });

        ForwardLightingData data{};
        if (const auto* global = blackboard.TryGet<GlobalLightingBinding>())
            data.m_global = *global;
        if (const auto* local = blackboard.TryGet<LocalLightingBinding>())
            data.m_local = *local;
        if (const auto* binding = blackboard.TryGet<SunShadowBinding>())
            data.m_sunShadow = *binding;

        auto lighting = Core::Buffer::CreateStructured<ForwardLightingData>(graph.GetDevice(), "ForwardLighting", 1);
        if (data.m_local.m_clusters.IsValid())
        {
            graph.AddBufferDependency(lighting.Get(), data.m_local.m_clusters);
            graph.AddBufferDependency(lighting.Get(), data.m_local.m_masks);
        }

        if (sunShadow != nullptr)
        {
            graph.AddBufferDependency(lighting.Get(), sunShadow->m_data.Get());
            graph.AddBufferDependency(lighting.Get(), sunShadow->m_pageTable.Get());
        }

        FE_Verify(view.m_renderQueueUploader->Upload(graph, lighting.Get(), data));
        SceneRenderPass pass;
        pass.m_viewProjection = view.m_view->GetViewProjectionMatrix();
        pass.m_passDescToken = graph.AddBasePassDesc(targets);
        pass.m_drawTag = DrawTags::Transparent;
        pass.m_techniqueRole = "ForwardTransparency";
        pass.m_passData = lighting.Get();
        pass.m_independentCulling = true;
        if (const auto* pyramids = blackboard.TryGet<DepthPyramidPass::Output>())
            pass.m_depthPyramid = &pyramids->m_farthest;

        view.m_scene->GetModules().ForEachActive([&](SceneModuleBase& module) {
            module.AddRenderPasses(graph, blackboard, *view.m_renderQueueUploader, pass);
        });

        auto* resolve = graph.AllocatePassData<TransparencyResolvePassDesc>();
        resolve->m_pipeline = ResolveTransparencyPipeline::GetPipeline();
        resolve->m_constants.m_accumulation = graph.GetSRV(accumulation.Get());
        resolve->m_constants.m_revealage = graph.GetSRV(revealage.Get());
        resolve->m_constants.m_color = graph.GetUAV(view.m_mainColorTarget);
        resolve->m_constants.m_resolution = resolution;
        graph.AddDispatchPass("ResolveTransparency", resolve, Math::CeilDivide(resolution, 8u));
    }
} // namespace FE::Graphics::ForwardTransparency
