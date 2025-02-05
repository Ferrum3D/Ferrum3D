#include <Graphics/RHI/Vulkan/Device.h>
#include <Graphics/RHI/Vulkan/ResourceBarrierBatcher.h>

namespace FE::Graphics::Vulkan
{
    namespace
    {
        VkAccessFlags ConvertBufferAccessFlags(uint64_t access)
        {
            if (access < festd::to_underlying(RHI::BufferWriteType::kCount))
            {
                switch (static_cast<RHI::BufferWriteType>(access))
                {
                case RHI::BufferWriteType::kTransferDestination:
                    return VK_ACCESS_TRANSFER_WRITE_BIT;

                case RHI::BufferWriteType::kUnorderedAccess:
                    return VK_ACCESS_MEMORY_READ_BIT;

                default:
                    FE_DebugBreak();
                    return 0;
                }
            }

            switch (static_cast<RHI::BufferReadType>(access))
            {
            case RHI::BufferReadType::kTransferSource:
                return VK_ACCESS_TRANSFER_READ_BIT;

            case RHI::BufferReadType::kShaderResource:
                return VK_ACCESS_SHADER_READ_BIT;

            case RHI::BufferReadType::kIndirectArgument:
                return VK_ACCESS_INDIRECT_COMMAND_READ_BIT;

            default:
                FE_DebugBreak();
                return 0;
            }
        }


        struct AccessFlagsWithLayout final
        {
            VkAccessFlags m_accessFlags;
            VkImageLayout m_imageLayout;
        };


        AccessFlagsWithLayout ConvertImageAccessFlags(uint64_t access)
        {
            if (access < festd::to_underlying(RHI::ImageWriteType::kCount))
            {
                switch (static_cast<RHI::ImageWriteType>(access))
                {
                case RHI::ImageWriteType::kTransferDestination:
                    return { VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL };

                case RHI::ImageWriteType::kUnorderedAccess:
                    return { VK_ACCESS_MEMORY_READ_BIT, VK_IMAGE_LAYOUT_GENERAL };

                case RHI::ImageWriteType::kColorTarget:
                    return { VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

                case RHI::ImageWriteType::kDepthStencilTarget:
                    return { VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

                default:
                    FE_DebugBreak();
                    return { 0, VK_IMAGE_LAYOUT_UNDEFINED };
                }
            }

            switch (static_cast<RHI::ImageReadType>(access))
            {
            case RHI::ImageReadType::kTransferSource:
                return { VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL };

            case RHI::ImageReadType::kShaderResource:
                return { VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

            case RHI::ImageReadType::kDepthRead:
                return { VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL };

            default:
                FE_DebugBreak();
                return { 0, VK_IMAGE_LAYOUT_UNDEFINED };
            }
        }
    } // namespace

    void ResourceBarrierBatcher::AddBarrier(const BufferBarrierDesc& desc)
    {
        if (!m_bufferBarriers.empty() && m_bufferBarriers.back().m_hash == desc.GetHash())
            return;

        m_bufferBarriers.push_back({ desc, desc.GetHash() });
    }


    void ResourceBarrierBatcher::AddBarrier(const ImageBarrierDesc& desc)
    {
        if (!m_imageBarriers.empty() && m_imageBarriers.back().m_hash == desc.GetHash())
            return;

        m_imageBarriers.push_back({ desc, desc.GetHash() });
    }


    void ResourceBarrierBatcher::Flush(VkCommandBuffer commandBuffer)
    {
        festd::small_vector<VkBufferMemoryBarrier> nativeBufferBarriers;
        nativeBufferBarriers.reserve(m_bufferBarriers.size());
        for (const auto& [barrierDesc, hash] : m_bufferBarriers)
        {
            VkBufferMemoryBarrier& barrier = nativeBufferBarriers.emplace_back();
            barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            barrier.buffer = barrierDesc.m_buffer;
            barrier.srcAccessMask = ConvertBufferAccessFlags(barrierDesc.m_sourceAccess);
            barrier.dstAccessMask = ConvertBufferAccessFlags(barrierDesc.m_destAccess);
            barrier.srcQueueFamilyIndex = m_device->GetQueueFamilyIndex(barrierDesc.m_sourceQueueKind);
            barrier.dstQueueFamilyIndex = m_device->GetQueueFamilyIndex(barrierDesc.m_destQueueKind);
            barrier.offset = 0;
            barrier.size = VK_WHOLE_SIZE;
        }

        festd::small_vector<VkImageMemoryBarrier> nativeImageBarriers;
        nativeImageBarriers.reserve(m_imageBarriers.size());
        for (const auto& [barrierDesc, hash] : m_imageBarriers)
        {
            VkImageMemoryBarrier& barrier = nativeImageBarriers.emplace_back();
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.image = barrierDesc.m_image;

            const auto [srcAccessFlags, oldLayout] = ConvertImageAccessFlags(barrierDesc.m_sourceAccess);
            barrier.srcAccessMask = srcAccessFlags;
            barrier.oldLayout = oldLayout;

            const auto [dstAccessFlags, newLayout] = ConvertImageAccessFlags(barrierDesc.m_destAccess);
            barrier.dstAccessMask = dstAccessFlags;
            barrier.newLayout = newLayout;
        }

        vkCmdPipelineBarrier(commandBuffer,
                             VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                             VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                             VK_DEPENDENCY_BY_REGION_BIT,
                             0,
                             nullptr,
                             nativeBufferBarriers.size(),
                             nativeBufferBarriers.data(),
                             nativeImageBarriers.size(),
                             nativeImageBarriers.data());
    }
} // namespace FE::Graphics::Vulkan
