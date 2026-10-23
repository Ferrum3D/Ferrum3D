#include <Graphics/Passes/DrawTags.h>
#include <Graphics/Passes/RendererPassCommon.h>
#include <Graphics/Passes/Tools/DepthPyramid.h>
#include <Graphics/Passes/VisibilityPass.h>
#include <Graphics/Scene/Culling.h>

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


    namespace
    {
        void AddScenePasses(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard, const RendererViewData& view,
                            const SceneRenderPass& pass)
        {
            view.m_scene->GetModules().ForEachActive([&](SceneModuleBase& module) {
                module.AddRenderPasses(graph, blackboard, *view.m_renderQueueUploader, pass);
            });
        }
    } // namespace


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
        const Culling::PassData& culling = Culling::GetPassData(blackboard);
        const bool isOcclusionCulled = culling.m_settings.IsOcclusionCullingEnabled();
        const Tools::DepthPyramid::Output* frozenDepthPyramid = culling.GetFrozenDepthPyramid();
        SceneRenderPass pass;
        pass.m_viewProjection = view.m_view->GetViewProjectionMatrix();
        pass.m_passDescToken = graph.AddBasePassDesc(coverage);
        pass.m_drawTag = data->m_drawTag;
        pass.m_techniqueRole = data->m_techniqueRole;
        pass.m_reportsStats = true;
        if (frozenDepthPyramid != nullptr || !isOcclusionCulled)
        {
            // A frozen camera has no visibility history to render first; its captured pyramid already holds the occluders.
            pass.m_phase = SceneRenderPhase::kSingle;
            pass.m_depthPyramid = isOcclusionCulled ? frozenDepthPyramid : nullptr;
            AddScenePasses(graph, blackboard, view, pass);
        }
        else
        {
            pass.m_phase = SceneRenderPhase::kEarly;
            AddScenePasses(graph, blackboard, view, pass);

            // The late phase tests all objects against the depth the early phase produced.
            Tools::DepthPyramid::Settings settings;
            settings.m_buildsClosest = false;
            const auto depthPyramids = Tools::DepthPyramid::AddPass(graph, view.m_mainDepthTarget, settings);
            pass.m_phase = SceneRenderPhase::kLate;
            pass.m_depthPyramid = &depthPyramids.m_farthest;
            AddScenePasses(graph, blackboard, view, pass);
        }

        // The first frozen frame captures the complete depth of the frozen camera.
        if (culling.m_frozenDepthPyramid != nullptr && frozenDepthPyramid == nullptr)
        {
            Tools::DepthPyramid::Settings settings;
            settings.m_buildsClosest = false;
            settings.m_isPersistent = true;
            *culling.m_frozenDepthPyramid = Tools::DepthPyramid::AddPass(graph, view.m_mainDepthTarget, settings).m_farthest;
        }
    }
} // namespace FE::Graphics::Visibility
