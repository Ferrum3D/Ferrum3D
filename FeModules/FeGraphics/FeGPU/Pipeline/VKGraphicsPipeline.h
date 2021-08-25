#pragma once
#include <FeGPU/Pipeline/IGraphicsPipeline.h>
#include <FeGPU/Common/VKConfig.h>

namespace FE::GPU
{
    class VKDevice;

    class VKGraphicsPipeline : public Object<IGraphicsPipeline>
    {
        VKDevice* m_Device;

        vk::UniquePipelineLayout m_Layout;
        vk::UniquePipeline m_NativePipeline;

    public:
        VKGraphicsPipeline(VKDevice& dev, const GraphicsPipelineDesc& desc);
    };
}
