#include <Graphics/Passes/DrawTags.h>
#include <Graphics/Passes/OpaquePass.h>
#include <Graphics/Passes/RendererPassCommon.h>

namespace FE::Graphics::OpaquePass
{
    ViewModule::ViewModule(View* view)
        : ViewModuleBase(view)
        , m_drawTag(DrawTags::Opaque)
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
        if (!blackboard.Contains<PassData>())
            return;

        const PassData& data = blackboard.Get<PassData>();
        const RendererViewData& viewData = blackboard.Get<RendererViewData>();

        auto* passDesc = graph.AllocatePassData<PassDesc>();
        passDesc->m_colorTarget = Core::TextureView::Create(viewData.m_mainColorTarget);
        passDesc->m_depthTarget = Core::TextureView::Create(viewData.m_mainDepthTarget);
        passDesc->m_viewport = viewData.m_viewportRect;

        SceneRenderPass pass;
        pass.m_viewProjection = viewData.m_view->GetViewProjectionMatrix();
        pass.m_passDescToken = graph.AddBasePassDesc(passDesc);
        pass.m_drawTag = data.m_drawTag;
        pass.m_techniqueRole = data.m_techniqueRole;

        viewData.m_scene->GetModules().ForEachActive([&](SceneModuleBase& module) {
            module.AddRenderPasses(graph, *viewData.m_renderQueueUploader, pass);
        });
    }
} // namespace FE::Graphics::OpaquePass
