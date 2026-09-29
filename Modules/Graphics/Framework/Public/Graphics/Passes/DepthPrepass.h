#pragma once
#include <Graphics/Core/FrameGraph/Blackboard.h>
#include <Graphics/Core/FrameGraph/FrameGraph.h>
#include <Graphics/Core/FrameGraph/FrameGraphPass.h>
#include <Graphics/Scene/Scene.h>
#include <Graphics/Scene/View.h>

namespace FE::Graphics::DepthPrepass
{
    struct PassDesc final
    {
        Core::PassDepthTarget m_depthTarget;
        Core::PassViewport m_viewport;

        FE_RTTI_Reflect("31472B80-2096-42D6-940D-A19F43E5795E");
    };


    struct PassData final
    {
        FE_RTTI_Reflect("6AE6D9A2-F171-4552-A2B4-208BECB536AA");
    };

    struct ViewModule final : public ViewModuleBase
    {
        FE_RTTI("682BB365-E2A5-46D7-A859-D9621F16AAE1");

        explicit ViewModule(View* view);
        ~ViewModule() override;

        void Update(Core::FrameGraphBlackboard& blackboard) override;

    private:
        void DoRelease() override;
    };

    void AddPasses(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard);
} // namespace FE::Graphics::DepthPrepass
