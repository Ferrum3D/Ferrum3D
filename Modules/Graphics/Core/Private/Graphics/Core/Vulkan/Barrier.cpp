#include <Graphics/Core/Vulkan/Barrier.h>
#include <Graphics/Core/Vulkan/Buffer.h>
#include <Graphics/Core/Vulkan/Device.h>
#include <Graphics/Core/Vulkan/Texture.h>

namespace FE::Graphics::Vulkan
{
    VkPipelineStageFlags2 TranslateSyncFlags(const Core::BarrierSyncFlags flags)
    {
        if (flags == Core::BarrierSyncFlags::kAll)
            return VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

        FE_Assert(!Bit::AnySet(flags, Core::BarrierSyncFlags::kAll), "All is only allowed as a single flag");

        VkPipelineStageFlags2 vkFlags = 0;
        if (Bit::AllSet(flags, Core::BarrierSyncFlags::kExecuteIndirect))
            vkFlags |= VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
        if (Bit::AllSet(flags, Core::BarrierSyncFlags::kVertexInput))
            vkFlags |= VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;
        if (Bit::AllSet(flags, Core::BarrierSyncFlags::kAmplificationShading))
            vkFlags |= VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_EXT;
        if (Bit::AllSet(flags, Core::BarrierSyncFlags::kMeshShading))
            vkFlags |= VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_EXT;
        if (Bit::AllSet(flags, Core::BarrierSyncFlags::kVertexShading))
            vkFlags |= VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
        if (Bit::AllSet(flags, Core::BarrierSyncFlags::kPixelShading))
            vkFlags |= VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
        if (Bit::AllSet(flags, Core::BarrierSyncFlags::kDepthStencil))
            vkFlags |= VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
        if (Bit::AllSet(flags, Core::BarrierSyncFlags::kRenderTarget))
            vkFlags |= VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        if (Bit::AllSet(flags, Core::BarrierSyncFlags::kComputeShading))
            vkFlags |= VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
        if (Bit::AllSet(flags, Core::BarrierSyncFlags::kCopy))
            vkFlags |= VK_PIPELINE_STAGE_2_COPY_BIT;
        if (Bit::AllSet(flags, Core::BarrierSyncFlags::kResolve))
            vkFlags |= VK_PIPELINE_STAGE_2_RESOLVE_BIT;
        if (Bit::AllSet(flags, Core::BarrierSyncFlags::kHost))
            vkFlags |= VK_PIPELINE_STAGE_2_HOST_BIT;

        return vkFlags;
    }


    VkAccessFlags2 TranslateAccessFlags(const Core::BarrierAccessFlags flags)
    {
        VkAccessFlags2 vkFlags = 0;
        if (Bit::AllSet(flags, Core::BarrierAccessFlags::kIndexBuffer))
            vkFlags |= VK_ACCESS_2_INDEX_READ_BIT;
        if (Bit::AllSet(flags, Core::BarrierAccessFlags::kVertexBuffer))
            vkFlags |= VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;
        if (Bit::AllSet(flags, Core::BarrierAccessFlags::kConstantBuffer))
            vkFlags |= VK_ACCESS_2_UNIFORM_READ_BIT;
        if (Bit::AllSet(flags, Core::BarrierAccessFlags::kIndirectArgument))
            vkFlags |= VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
        if (Bit::AllSet(flags, Core::BarrierAccessFlags::kRenderTarget))
            vkFlags |= VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
        if (Bit::AllSet(flags, Core::BarrierAccessFlags::kShaderRead))
            vkFlags |= VK_ACCESS_2_SHADER_READ_BIT;
        if (Bit::AllSet(flags, Core::BarrierAccessFlags::kShaderWrite))
            vkFlags |= VK_ACCESS_2_SHADER_WRITE_BIT;
        if (Bit::AllSet(flags, Core::BarrierAccessFlags::kDepthStencilRead))
            vkFlags |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
        if (Bit::AllSet(flags, Core::BarrierAccessFlags::kDepthStencilWrite))
            vkFlags |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        if (Bit::AnySet(flags, Core::BarrierAccessFlags::kCopySource | Core::BarrierAccessFlags::kResolveSource))
            vkFlags |= VK_ACCESS_2_TRANSFER_READ_BIT;
        if (Bit::AllSet(flags, Core::BarrierAccessFlags::kCopyDest | Core::BarrierAccessFlags::kResolveDest))
            vkFlags |= VK_ACCESS_2_TRANSFER_WRITE_BIT;
        if (Bit::AllSet(flags, Core::BarrierAccessFlags::kShadingRateSource))
            vkFlags |= VK_ACCESS_2_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR;
        if (Bit::AllSet(flags, Core::BarrierAccessFlags::kAccelerationStructureRead))
            vkFlags |= VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR;
        if (Bit::AllSet(flags, Core::BarrierAccessFlags::kAccelerationStructureWrite))
            vkFlags |= VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;

        return vkFlags;
    }


    VkImageLayout TranslateImageLayout(const Core::BarrierLayout layout)
    {
        switch (layout)
        {
        default:
            FE_DebugBreak();
            [[fallthrough]];
        case Core::BarrierLayout::kUndefined:
            return VK_IMAGE_LAYOUT_UNDEFINED;
        case Core::BarrierLayout::kRenderTarget:
            return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        case Core::BarrierLayout::kShaderRead:
            return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        case Core::BarrierLayout::kShaderReadWrite:
            return VK_IMAGE_LAYOUT_GENERAL;
        case Core::BarrierLayout::kDepthStencilRead:
            return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        case Core::BarrierLayout::kDepthStencilWrite:
            return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        case Core::BarrierLayout::kCopySource:
            return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        case Core::BarrierLayout::kCopyDest:
            return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        case Core::BarrierLayout::kResolveSource:
            return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        case Core::BarrierLayout::kResolveDest:
            return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        case Core::BarrierLayout::kShadingRateSource:
            return VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;
        case Core::BarrierLayout::kPresentSource:
            return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        }
    }


    VkImageMemoryBarrier2 TranslateBarrier(const Core::TextureBarrierDesc& barrier, const Device* device)
    {
        VkImageMemoryBarrier2 vkBarrier = {};
        vkBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        vkBarrier.srcStageMask = TranslateSyncFlags(barrier.m_syncBefore);
        vkBarrier.dstStageMask = TranslateSyncFlags(barrier.m_syncAfter);
        vkBarrier.srcAccessMask = TranslateAccessFlags(barrier.m_accessBefore);
        vkBarrier.dstAccessMask = TranslateAccessFlags(barrier.m_accessAfter);
        vkBarrier.oldLayout = TranslateImageLayout(barrier.m_layoutBefore);
        vkBarrier.newLayout = TranslateImageLayout(barrier.m_layoutAfter);
        vkBarrier.image = NativeCast(barrier.m_texture);
        vkBarrier.subresourceRange = TranslateSubresourceRange(barrier.m_subresource);

        if (barrier.m_queueBefore != barrier.m_queueAfter)
        {
            const uint32_t queueFamilyIndexBefore = device->GetQueueFamilyIndex(barrier.m_queueBefore);
            const uint32_t queueFamilyIndexAfter = device->GetQueueFamilyIndex(barrier.m_queueAfter);

            vkBarrier.srcQueueFamilyIndex = queueFamilyIndexBefore;
            vkBarrier.dstQueueFamilyIndex = queueFamilyIndexAfter;
        }
        else
        {
            vkBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            vkBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        }

        return vkBarrier;
    }


    VkBufferMemoryBarrier2 TranslateBarrier(const Core::BufferBarrierDesc& barrier, const Device* device)
    {
        VkBufferMemoryBarrier2 vkBarrier = {};
        vkBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
        vkBarrier.srcStageMask = TranslateSyncFlags(barrier.m_syncBefore);
        vkBarrier.dstStageMask = TranslateSyncFlags(barrier.m_syncAfter);
        vkBarrier.srcAccessMask = TranslateAccessFlags(barrier.m_accessBefore);
        vkBarrier.dstAccessMask = TranslateAccessFlags(barrier.m_accessAfter);
        vkBarrier.buffer = NativeCast(barrier.m_buffer);
        vkBarrier.offset = 0;
        vkBarrier.size = barrier.m_buffer->GetDesc().m_size;

        if (barrier.m_queueBefore != barrier.m_queueAfter)
        {
            const uint32_t queueFamilyIndexBefore = device->GetQueueFamilyIndex(barrier.m_queueBefore);
            const uint32_t queueFamilyIndexAfter = device->GetQueueFamilyIndex(barrier.m_queueAfter);

            vkBarrier.srcQueueFamilyIndex = queueFamilyIndexBefore;
            vkBarrier.dstQueueFamilyIndex = queueFamilyIndexAfter;
        }
        else
        {
            vkBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            vkBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        }

        return vkBarrier;
    }
} // namespace FE::Graphics::Vulkan
