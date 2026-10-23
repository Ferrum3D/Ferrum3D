#pragma once
#include <Graphics/Passes/Tools/DepthPyramid.h>
#include <Graphics/Scene/View.h>

namespace FE::Graphics::DepthPyramidPass
{
    //! Published by AddPasses: the farthest and closest pyramids of the view's complete depth after visibility rasterization.
    struct Output final
    {
        Tools::DepthPyramid::Output m_farthest;
        Tools::DepthPyramid::Output m_closest;
    };


    struct PassData final
    {
    };


    //! Builds both depth pyramids of the view for later passes, e.g. screen-space effects.
    struct ViewModule final : public ViewModuleBase
    {
        FE_RTTI("1F8CB52B-6D21-4C51-BB84-31D5ADA1C348");

        explicit ViewModule(View* view);
        ~ViewModule() override;

        void Update(Core::FrameGraphBlackboard& blackboard) override;

    private:
        void DoRelease() override;
    };


    void AddPasses(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard);
} // namespace FE::Graphics::DepthPyramidPass
