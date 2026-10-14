#pragma once
#include <Graphics/Core/FrameGraph/FrameGraph.h>

namespace FE::Graphics::Visibility
{
    struct CoveragePassDesc final
    {
        Core::PassColorTarget m_visibility;
        Core::PassDepthTarget m_depth;
        Core::PassViewport m_viewport;

        FE_RTTI_Reflect("6F735FDA-2C65-4F81-992E-B40E13F7FBBF");
    };


    void AddPasses(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard, Core::Texture* visibility);
} // namespace FE::Graphics::Visibility
