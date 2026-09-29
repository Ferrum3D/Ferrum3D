#pragma once
#include <Graphics/Core/FrameGraph/Blackboard.h>
#include <Graphics/Core/FrameGraph/FrameGraph.h>
#include <Graphics/Core/FrameGraph/FrameGraphPass.h>
#include <Graphics/Scene/Scene.h>
#include <Graphics/Scene/View.h>

namespace FE::Graphics::OpaquePass
{
    struct PassDesc final
    {
        Core::PassColorTarget m_colorTarget;
        Core::PassDepthTarget m_depthTarget;
        Core::PassViewport m_viewport;

        FE_RTTI_Reflect("86A42C8E-A926-4E59-98EF-17AA0F79A718");
    };


    struct PassData final
    {
        FE_RTTI_Reflect("B27600BC-3187-49D2-A81F-254D6E0C606C");
    };

    struct ViewModule final : public ViewModuleBase
    {
        FE_RTTI("38D17F97-8B8F-40E0-AEF7-3B828C04182E");

        explicit ViewModule(View* view);
        ~ViewModule() override;

        void Update(Core::FrameGraphBlackboard& blackboard) override;

    private:
        void DoRelease() override;
    };

    void AddPasses(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard);
} // namespace FE::Graphics::OpaquePass
