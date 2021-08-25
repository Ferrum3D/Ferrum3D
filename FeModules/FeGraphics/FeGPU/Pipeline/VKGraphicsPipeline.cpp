#include <FeGPU/Pipeline/VKGraphicsPipeline.h>
#include <FeGPU/Device/VKDevice.h>

namespace FE::GPU
{
    VKGraphicsPipeline::VKGraphicsPipeline(VKDevice& dev, const GraphicsPipelineDesc& desc)
        : m_Device(&dev)
    {

    }
}
