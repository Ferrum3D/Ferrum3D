#include <Graphics/RHI/Vulkan/FrameGraph/FrameGraph.h>

namespace FE::Graphics::Vulkan
{
    FrameGraph::FrameGraph(RHI::Device* device)
    {
        m_device = device;
    }
} // namespace FE::Graphics::Vulkan
