#include <Graphics/Passes/RendererPassCommon.h>
#include <Graphics/Passes/TonemapPass.h>
#include <Graphics/Passes/Tools/Tonemap.h>

namespace FE::Graphics::Tonemap
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
        blackboard.Add<ViewModule*>(this);
    }


    void AddPasses(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard)
    {
        if (!blackboard.Contains<ViewModule*>())
            return;

        // Views whose scene passes are missing or skipped still present the background.
        const RendererViewData& view = blackboard.Get<RendererViewData>();
        if (!view.m_isMainColorWritten)
            ClearMainTargets(graph, view);
    }


    void AddOutputPasses(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard)
    {
        if (!blackboard.Contains<ViewModule*>())
            return;

        const RendererViewData& view = blackboard.Get<RendererViewData>();
        ViewOutput& output = blackboard.Get<ViewOutput>();
        Tools::Tonemap::AddPass(graph, Core::TextureView::Create(view.m_mainColorTarget), output.m_target);
        output.m_isInitialized = true;
    }
} // namespace FE::Graphics::Tonemap
