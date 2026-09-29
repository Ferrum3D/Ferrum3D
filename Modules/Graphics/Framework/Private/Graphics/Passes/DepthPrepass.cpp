#include <Graphics/Passes/DepthPrepass.h>
#include <Graphics/Passes/DrawTags.h>
#include <Graphics/Passes/RendererPassCommon.h>

namespace FE::Graphics::DepthPrepass
{
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
        blackboard.Add<PassData>();
    }


    void AddPasses(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard)
    {
        if (!blackboard.Contains<PassData>())
            return;

        const RendererViewData& viewData = blackboard.Get<RendererViewData>();
        auto* passDesc = graph.AllocatePassData<PassDesc>();
        passDesc->m_depthTarget = Core::TextureView::Create(viewData.m_mainDepthTarget);
        passDesc->m_viewport = viewData.m_viewportRect;
        const Core::BasePassDescToken token = graph.AddBasePassDesc(passDesc);
        SceneRenderPass pass;
        pass.m_viewProjection = viewData.m_view->GetViewProjectionMatrix();
        pass.m_passDescToken = token;
        pass.m_drawTag = DrawTags::DepthPrepass;
        pass.m_techniqueRole = "DepthOnly";

        viewData.m_scene->GetModules().ForEachActive([&](SceneModuleBase& module) {
            module.AddRenderPasses(graph, *viewData.m_renderQueueUploader, pass);
        });
    }
} // namespace FE::Graphics::DepthPrepass
