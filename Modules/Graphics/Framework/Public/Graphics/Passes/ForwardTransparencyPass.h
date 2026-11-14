#pragma once
#include <Graphics/Scene/View.h>

namespace FE::Graphics::ForwardTransparency
{
    //! ForwardTransparency techniques must declare these color targets in order: RGBA16F, R16F, R8_UNORM.
    struct Targets final
    {
        Core::PassColorTarget m_accumulation;
        Core::PassColorTarget m_revealage;
        Core::PassColorTarget m_reactive;
        Core::PassDepthTarget m_depth;
        Core::PassViewport m_viewport;
        FE_RTTI_Reflect("0DA39110-5A90-4664-A2C1-E8D71488C82C");
    };


    struct Output final
    {
        //! Opacity of the transparent layers, for temporal modules whose history must yield to them.
        Rc<Core::Texture> m_reactive;
    };


    struct ViewModule final : ViewModuleBase
    {
        FE_RTTI("892AE3B6-BBE9-4BD6-82DC-5346AC889AC3");
        explicit ViewModule(View* view);
        void Update(Core::FrameGraphBlackboard& blackboard) override;

    private:
        void DoRelease() override;
    };


    void AddPasses(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard);
} // namespace FE::Graphics::ForwardTransparency
