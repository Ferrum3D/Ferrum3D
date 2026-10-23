#include <Graphics/Passes/DepthPyramidPass.h>
#include <Graphics/Passes/RendererPassCommon.h>
#include <Graphics/Passes/VisibilityPass.h>

namespace FE::Graphics::DepthPyramidPass
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
        // The pyramids reduce the depth that visibility rasterization wrote.
        if (blackboard.TryGet<PassData>() == nullptr || blackboard.TryGet<Visibility::Output>() == nullptr)
            return;

        const RendererViewData& view = blackboard.Get<RendererViewData>();
        const auto pyramids = Tools::DepthPyramid::AddPass(graph, view.m_mainDepthTarget);

        Output& output = blackboard.Add<Output>();
        output.m_farthest = pyramids.m_farthest;
        output.m_closest = pyramids.m_closest;
    }
} // namespace FE::Graphics::DepthPyramidPass
