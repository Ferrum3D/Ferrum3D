#pragma once
#include <Graphics/Core/FrameGraph/FrameGraph.h>
#include <Graphics/Scene/View.h>

namespace FE::Graphics::DeferredShading
{
    struct ViewModule final : public ViewModuleBase
    {
        FE_RTTI("3F99C20A-F2DE-442A-AC4E-930462412DBD");

        explicit ViewModule(View* view);
        ~ViewModule() override;
        void Update(Core::FrameGraphBlackboard& blackboard) override;

    private:
        void DoRelease() override;
    };


    void AddPasses(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard);
} // namespace FE::Graphics::DeferredShading
