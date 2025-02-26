#include <FeCore/DI/Activator.h>
#include <Graphics/RHI/Vulkan/Device.h>
#include <Graphics/RHI/Vulkan/GraphicsPipeline.h>
#include <Graphics/RHI/Vulkan/PipelineFactory.h>

namespace FE::Graphics::Vulkan
{
    PipelineFactory::PipelineFactory(RHI::Device* device, Logger* logger)
        : m_graphicsPipelinePool("GraphicsPipelinePool", sizeof(GraphicsPipeline))
        , m_logger(logger)
    {
        m_device = device;

        m_logger->LogTrace("Creating Pipeline Factory");

        VkPipelineCacheCreateInfo pipelineCacheCI{};
        pipelineCacheCI.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        pipelineCacheCI.initialDataSize = 0;
        pipelineCacheCI.pInitialData = nullptr;

        const VkResult result = vkCreatePipelineCache(NativeCast(device), &pipelineCacheCI, nullptr, &m_pipelineCache);
        FE_Assert(result == VK_SUCCESS);
    }


    PipelineFactory::~PipelineFactory()
    {
        if (m_pipelineCache)
            vkDestroyPipelineCache(NativeCast(m_device), m_pipelineCache, nullptr);
    }


    RHI::GraphicsPipeline* PipelineFactory::CreateGraphicsPipeline(const RHI::GraphicsPipelineRequest& request)
    {
        GraphicsPipeline* pipeline = DI::New<GraphicsPipeline>(&m_graphicsPipelinePool).value();
        const RHI::ResultCode result = pipeline->InitInternal(m_pipelineCache, request.m_pipelineName, request.m_desc);
        FE_Assert(result == RHI::ResultCode::kSuccess);

        return pipeline;
    }


    void PipelineFactory::DispatchPending()
    {
        // TODO
    }
} // namespace FE::Graphics::Vulkan
