#pragma once
#include <Graphics/RHI/Common/FrameGraph/FrameGraph.h>

namespace FE::Graphics::Vulkan
{
    struct FrameGraph final : public Common::FrameGraph
    {
        FrameGraph(RHI::Device* device);

    private:
    };
} // namespace FE::Graphics::Vulkan
