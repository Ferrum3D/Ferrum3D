#pragma once
#include <FeCore/Memory/PoolAllocator.h>
#include <Graphics/RHI/PipelineFactory.h>
#include <Graphics/RHI/Vulkan/Base/Config.h>

namespace FE::Graphics::Vulkan
{
    struct PipelineFactory final : RHI::PipelineFactory
    {
        PipelineFactory(RHI::Device* device, Logger* logger);
        ~PipelineFactory() override;

        FE_RTTI_Class(PipelineFactory, "437E4387-BDE0-42DA-8986-FA909D8BFEDE");

        festd::expected<RHI::GraphicsPipeline*, RHI::ResultCode> CreateGraphicsPipeline(
            Env::Name name, const RHI::GraphicsPipelineDesc& desc) override;

    private:
        Memory::PoolAllocator m_graphicsPipelinePool;
        Logger* m_logger = nullptr;
        VkPipelineCache m_pipelineCache = VK_NULL_HANDLE;
    };
} // namespace FE::Graphics::Vulkan
