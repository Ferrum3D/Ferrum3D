#pragma once
#include <Graphics/Core/FrameGraph/Blackboard.h>
#include <Graphics/Core/FrameGraph/FrameGraph.h>
#include <Graphics/Scene/Scene.h>
#include <Graphics/Scene/View.h>

namespace FE::Graphics::OpaquePass
{
    struct PassData final
    {
    };

    struct ViewModule final : public ViewModuleBase
    {
        FE_RTTI("38D17F97-8B8F-40E0-AEF7-3B828C04182E");

        explicit ViewModule(View* view);
        ~ViewModule() override;

        void Update(Core::FrameGraphBlackboard& blackboard) override;
    };

    void AddPasses(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard, Scene& scene);
} // namespace FE::Graphics::OpaquePass

FE_DECLARE_PASS_DATA(FE::Graphics::OpaquePass::PassData);
