#include <Graphics/Passes/DrawTags.h>
#include <Graphics/Passes/OpaquePass.h>
#include <Graphics/Passes/RendererPassCommon.h>

namespace FE::Graphics::OpaquePass
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
        SceneRenderPass pass;
        pass.m_viewProjection = viewData.m_view->GetViewProjectionMatrix();
        pass.m_colorTarget = viewData.m_mainColorTarget;
        pass.m_depthTarget = viewData.m_mainDepthTarget;
        pass.m_viewport = viewData.m_viewportRect;
        pass.m_drawTag = DrawTags::Opaque;
        pass.m_techniqueRole = "Opaque";

        viewData.m_scene->GetModules().ForEachActive([&](SceneModuleBase& module) {
            module.AddRenderPasses(graph, *viewData.m_renderQueueUploader, pass);
        });
    }
} // namespace FE::Graphics::OpaquePass
