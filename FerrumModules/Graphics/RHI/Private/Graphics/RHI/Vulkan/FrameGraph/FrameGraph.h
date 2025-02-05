#pragma once
#include <Graphics/RHI/FrameGraph/FrameGraph.h>

namespace FE::Graphics::Vulkan
{
    struct FrameGraph final : public RHI::FrameGraph
    {
        FrameGraph(RHI::Device* device);

    private:
    };
} // namespace FE::Graphics::Vulkan
