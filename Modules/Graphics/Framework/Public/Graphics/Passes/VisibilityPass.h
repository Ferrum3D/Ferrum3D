#pragma once
#include <Graphics/Base/DrawTag.h>
#include <Graphics/Core/FrameGraph/FrameGraph.h>
#include <Graphics/Core/Texture.h>
#include <Graphics/Scene/View.h>

namespace FE::Graphics::Visibility
{
    struct CoveragePassDesc final
    {
        Core::PassColorTarget m_visibility;
        Core::PassDepthTarget m_depth;
        Core::PassViewport m_viewport;
        FE_RTTI_Reflect("6F735FDA-2C65-4F81-992E-B40E13F7FBBF");
    };


    struct PassData final
    {
        DrawTag m_drawTag;
        Env::Name m_techniqueRole;
    };


    struct Output final
    {
        Rc<Core::Texture> m_target;
    };


    struct ViewModule final : public ViewModuleBase
    {
        FE_RTTI("D6F76E41-81B5-4442-B976-E9F80B37FEB7");

        DrawTag m_drawTag;
        Env::Name m_techniqueRole = "DepthVisibility";

        explicit ViewModule(View* view);
        ~ViewModule() override;
        void Update(Core::FrameGraphBlackboard& blackboard) override;

    private:
        void DoRelease() override;
    };


    void AddPasses(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard);
} // namespace FE::Graphics::Visibility
