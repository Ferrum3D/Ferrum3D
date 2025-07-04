#include <Graphics/Core/FrameGraph/Base.h>
#include <Graphics/Core/Vulkan/ResourceBarrierBatcher.h>

namespace FE::Graphics::Vulkan
{
    namespace
    {
        VkAccessFlags2 ConvertBufferAccessFlags(const uint64_t access)
        {
            if (access < festd::to_underlying(Core::BufferWriteType::kCount))
            {
                switch (static_cast<Core::BufferWriteType>(access))
                {
                case Core::BufferWriteType::kTransferDestination:
                    return VK_ACCESS_2_TRANSFER_WRITE_BIT;

                case Core::BufferWriteType::kUnorderedAccess:
                    return VK_ACCESS_2_MEMORY_READ_BIT;

                case Core::BufferWriteType::kCount:
                default:
                    FE_DebugBreak();
                    return 0;
                }
            }

            switch (static_cast<Core::BufferReadType>(access))
            {
            case Core::BufferReadType::kTransferSource:
                return VK_ACCESS_2_TRANSFER_READ_BIT;

            case Core::BufferReadType::kShaderResource:
                return VK_ACCESS_2_SHADER_READ_BIT;

            case Core::BufferReadType::kIndirectArgument:
                return VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;

            case Core::BufferReadType::kCount:
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


        AccessFlagsWithLayout ConvertImageAccessFlags(const uint64_t access)
        {
            if (access < festd::to_underlying(Core::ImageWriteType::kCount))
            {
                switch (static_cast<Core::ImageWriteType>(access))
                {
                case Core::ImageWriteType::kTransferDestination:
                    return { VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL };

                case Core::ImageWriteType::kUnorderedAccess:
                    return { VK_ACCESS_MEMORY_READ_BIT, VK_IMAGE_LAYOUT_GENERAL };

                case Core::ImageWriteType::kColorTarget:
                    return { VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

                case Core::ImageWriteType::kDepthStencilTarget:
                    return { VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

                case Core::ImageWriteType::kCount:
                default:
                    FE_DebugBreak();
                    return { 0, VK_IMAGE_LAYOUT_UNDEFINED };
                }
            }

            switch (static_cast<Core::ImageReadType>(access))
            {
            case Core::ImageReadType::kTransferSource:
                return { VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL };

            case Core::ImageReadType::kShaderResource:
                return { VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

            case Core::ImageReadType::kDepthRead:
                return { VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL };

            case Core::ImageReadType::kCount:
            default:
                FE_DebugBreak();
                return { 0, VK_IMAGE_LAYOUT_UNDEFINED };
            }
        }
    } // namespace


    void ResourceBarrierBatcher::Begin(const VkCommandBuffer commandBuffer)
    {
        FE_Assert(m_commandBuffer == VK_NULL_HANDLE);
        m_commandBuffer = commandBuffer;
    }


    void ResourceBarrierBatcher::AddBarrier(const BufferBarrierDesc& desc)
    {
        FE_PROFILER_ZONE();

        const uint64_t hash = desc.GetHash();

        bool needFlush = false;
        for (auto& [barrierDesc, barrierHash] : m_bufferBarriers)
        {
            if (barrierHash == hash)
                return;

            if (barrierDesc.m_buffer == desc.m_buffer)
            {
                needFlush = true;
                break;
            }
        }

        if (needFlush)
            Flush();

        m_bufferBarriers.push_back({ desc, hash });
    }


    void ResourceBarrierBatcher::AddBarrier(const ImageBarrierDesc& desc)
    {
        FE_PROFILER_ZONE();

        const uint64_t hash = desc.GetHash();

        bool needFlush = false;
        for (auto& [barrierDesc, barrierHash] : m_imageBarriers)
        {
            if (barrierHash == hash)
                return;

            if (barrierDesc.m_image == desc.m_image)
            {
                needFlush = true;
                break;
            }
        }

        if (needFlush)
            Flush();

        m_imageBarriers.push_back({ desc, hash });
    }


    void ResourceBarrierBatcher::Flush()
    {
        FE_PROFILER_ZONE();

        festd::small_vector<VkBufferMemoryBarrier2> nativeBufferBarriers;
        nativeBufferBarriers.reserve(m_bufferBarriers.size());
        for (const auto& [barrierDesc, hash] : m_bufferBarriers)
        {
            VkBufferMemoryBarrier2& barrier = nativeBufferBarriers.emplace_back();
            barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
            barrier.buffer = barrierDesc.m_buffer;
            barrier.srcAccessMask = ConvertBufferAccessFlags(barrierDesc.m_sourceAccess);
            barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            barrier.dstAccessMask = ConvertBufferAccessFlags(barrierDesc.m_destAccess);
            barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

            barrier.offset = 0;
            barrier.size = VK_WHOLE_SIZE;

            if (barrierDesc.m_sourceQueueKind != barrierDesc.m_destQueueKind)
            {
                barrier.srcQueueFamilyIndex = m_device->GetQueueFamilyIndex(barrierDesc.m_sourceQueueKind);
                barrier.dstQueueFamilyIndex = m_device->GetQueueFamilyIndex(barrierDesc.m_destQueueKind);
            }
            else
            {
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            }
        }

        festd::small_vector<VkImageMemoryBarrier2> nativeImageBarriers;
        nativeImageBarriers.reserve(m_imageBarriers.size());
        for (const auto& [barrierDesc, hash] : m_imageBarriers)
        {
            VkImageMemoryBarrier2& barrier = nativeImageBarriers.emplace_back();
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
            barrier.image = barrierDesc.m_image;

            const auto [srcAccessFlags, oldLayout] = ConvertImageAccessFlags(barrierDesc.m_sourceAccess);
            barrier.srcAccessMask = srcAccessFlags;
            barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            barrier.oldLayout = oldLayout;

            const auto [dstAccessFlags, newLayout] = ConvertImageAccessFlags(barrierDesc.m_destAccess);
            barrier.dstAccessMask = dstAccessFlags;
            barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            barrier.newLayout = newLayout;

            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = barrierDesc.m_subresource.m_mostDetailedMipSlice;
            barrier.subresourceRange.levelCount = barrierDesc.m_subresource.m_mipSliceCount;
            barrier.subresourceRange.baseArrayLayer = barrierDesc.m_subresource.m_firstArraySlice;
            barrier.subresourceRange.layerCount = barrierDesc.m_subresource.m_arraySize;

            if (barrierDesc.m_sourceQueueKind != barrierDesc.m_destQueueKind)
            {
                barrier.srcQueueFamilyIndex = m_device->GetQueueFamilyIndex(barrierDesc.m_sourceQueueKind);
                barrier.dstQueueFamilyIndex = m_device->GetQueueFamilyIndex(barrierDesc.m_destQueueKind);
            }
            else
            {
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            }
        }

        VkDependencyInfo dependencyInfo = {};
        dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dependencyInfo.bufferMemoryBarrierCount = nativeBufferBarriers.size();
        dependencyInfo.pBufferMemoryBarriers = nativeBufferBarriers.data();
        dependencyInfo.imageMemoryBarrierCount = nativeImageBarriers.size();
        dependencyInfo.pImageMemoryBarriers = nativeImageBarriers.data();

        vkCmdPipelineBarrier2(m_commandBuffer, &dependencyInfo);

        m_bufferBarriers.clear();
        m_imageBarriers.clear();
    }
} // namespace FE::Graphics::Vulkan
