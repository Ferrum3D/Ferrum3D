#pragma once
#include <Graphics/Core/FrameGraph/Blackboard.h>
#include <Graphics/Core/FrameGraph/FrameGraph.h>
#include <Graphics/Scene/Scene.h>
#include <Graphics/Scene/View.h>

namespace FE::Graphics::DepthPrepass
{
    struct PassData final
    {
    };

    struct ViewModule final : public ViewModuleBase
    {
        FE_RTTI("682BB365-E2A5-46D7-A859-D9621F16AAE1");

        explicit ViewModule(View* view);
        ~ViewModule() override;

        void Update(Core::FrameGraphBlackboard& blackboard) override;
    };

    void AddPasses(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard, Scene& scene);
} // namespace FE::Graphics::DepthPrepass

FE_DECLARE_PASS_DATA(FE::Graphics::DepthPrepass::PassData);
