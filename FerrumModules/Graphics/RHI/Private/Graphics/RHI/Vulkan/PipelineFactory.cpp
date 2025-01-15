#include <FeCore/DI/Activator.h>
#include <Graphics/RHI/Vulkan/Device.h>
#include <Graphics/RHI/Vulkan/GraphicsPipeline.h>
#include <Graphics/RHI/Vulkan/PipelineFactory.h>

namespace FE::Graphics::Vulkan
{
    PipelineFactory::PipelineFactory(RHI::Device* device, Logger* logger)
        : m_graphicsPipelinePool("GraphicsPipelinePool", sizeof(GraphicsPipeline), 64 * 1024)
        , m_logger(logger)
    {
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


    festd::expected<RHI::GraphicsPipeline*, RHI::ResultCode> PipelineFactory::CreateGraphicsPipeline(
        const Env::Name name, const RHI::GraphicsPipelineDesc& desc)
    {
        return DI::New<GraphicsPipeline>(&m_graphicsPipelinePool)
            .map_error([](DI::ResultCode) {
                return RHI::ResultCode::kUnknownError;
            })
            .and_then([this, name, &desc](GraphicsPipeline* pipeline) -> festd::expected<GraphicsPipeline*, RHI::ResultCode> {
                const RHI::ResultCode result = pipeline->InitInternal(m_pipelineCache, name, desc);
                if (result == RHI::ResultCode::kSuccess)
                    return pipeline;

                return festd::unexpected(result);
            });
    }
} // namespace FE::Graphics::Vulkan
