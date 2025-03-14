#include <Graphics/Core/FrameGraph/Base.h>
#include <Graphics/Core/Vulkan/ResourceBarrierBatcher.h>

namespace FE::Graphics::Vulkan
{
    namespace
    {
        VkAccessFlags ConvertBufferAccessFlags(const uint64_t access)
        {
            if (access < festd::to_underlying(Core::BufferWriteType::kCount))
            {
                switch (static_cast<Core::BufferWriteType>(access))
                {
                case Core::BufferWriteType::kTransferDestination:
                    return VK_ACCESS_TRANSFER_WRITE_BIT;

                case Core::BufferWriteType::kUnorderedAccess:
                    return VK_ACCESS_MEMORY_READ_BIT;

                case Core::BufferWriteType::kCount:
                default:
                    FE_DebugBreak();
                    return 0;
                }
            }

            switch (static_cast<Core::BufferReadType>(access))
            {
            case Core::BufferReadType::kTransferSource:
                return VK_ACCESS_TRANSFER_READ_BIT;

            case Core::BufferReadType::kShaderResource:
                return VK_ACCESS_SHADER_READ_BIT;

            case Core::BufferReadType::kIndirectArgument:
                return VK_ACCESS_INDIRECT_COMMAND_READ_BIT;

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


        bool IsBarrierCompatible(const BufferBarrierDesc& a, const BufferBarrierDesc& b)
        {
            if (a.m_buffer != b.m_buffer)
                return false;

            if (a.m_sourceQueueKind != b.m_sourceQueueKind || a.m_destQueueKind != b.m_destQueueKind)
                return false;

            if (a.m_sourceAccess != b.m_sourceAccess || a.m_destAccess != b.m_destAccess)
                return false;

            if (a.m_offset >= b.m_offset + b.m_size || b.m_offset >= a.m_offset + a.m_size)
                return false;

            return true;
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
        for (auto& [barrierDesc, barrierHash] : m_bufferBarriers)
        {
            if (barrierHash == hash)
                return;

            if (IsBarrierCompatible(barrierDesc, desc))
            {
                const uint32_t start = Math::Min(barrierDesc.m_offset, desc.m_offset);
                const uint32_t end = Math::Max(barrierDesc.m_offset + barrierDesc.m_size, desc.m_offset + desc.m_size);

                barrierDesc.m_offset = start;
                barrierDesc.m_size = end - start;
                barrierHash = barrierDesc.GetHash();
                return;
            }
        }

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
            barrier.offset = barrierDesc.m_offset;
            barrier.size = barrierDesc.m_size;
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

        vkCmdPipelineBarrier(m_commandBuffer,
                             VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                             VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                             VK_DEPENDENCY_BY_REGION_BIT,
                             0,
                             nullptr,
                             nativeBufferBarriers.size(),
                             nativeBufferBarriers.data(),
                             nativeImageBarriers.size(),
                             nativeImageBarriers.data());

        m_bufferBarriers.clear();
        m_imageBarriers.clear();
    }
} // namespace FE::Graphics::Vulkan
