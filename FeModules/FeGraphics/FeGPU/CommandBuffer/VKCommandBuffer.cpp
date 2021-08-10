#include <FeGPU/CommandBuffer/VKCommandBuffer.h>
#include <FeGPU/Device/VKDevice.h>
#include <FeGPU/Image/VKImage.h>

namespace FE::GPU
{
    vk::ImageLayout VKConvert(ResourceState state)
    {
        static auto Conversions = []() {
            std::array<vk::ImageLayout, static_cast<size_t>(Format::Count)> result{};
            result[static_cast<size_t>(ResourceState::None)]            = vk::ImageLayout::eUndefined;
            result[static_cast<size_t>(ResourceState::Common)]          = vk::ImageLayout::eGeneral;
            result[static_cast<size_t>(ResourceState::RenderTarget)]    = vk::ImageLayout::eColorAttachmentOptimal;
            result[static_cast<size_t>(ResourceState::UnorderedAccess)] = vk::ImageLayout::eGeneral;
            result[static_cast<size_t>(ResourceState::DepthRead)]       = vk::ImageLayout::eDepthStencilReadOnlyOptimal;
            result[static_cast<size_t>(ResourceState::DepthWrite)]      = vk::ImageLayout::eDepthStencilAttachmentOptimal;
            result[static_cast<size_t>(ResourceState::ShaderResource)]  = vk::ImageLayout::eShaderReadOnlyOptimal;
            result[static_cast<size_t>(ResourceState::CopySource)]      = vk::ImageLayout::eTransferSrcOptimal;
            result[static_cast<size_t>(ResourceState::CopyDest)]        = vk::ImageLayout::eTransferDstOptimal;
            result[static_cast<size_t>(ResourceState::Present)]         = vk::ImageLayout::ePresentSrcKHR;
            return result;
        }();

        return Conversions[static_cast<size_t>(state)];
    }

    inline vk::ImageAspectFlags VKConvert(ImageAspectFlags aspect)
    {
        vk::ImageAspectFlags result{};
        if ((aspect & ImageAspectFlags::Color) != ImageAspectFlags::None)
            result |= vk::ImageAspectFlagBits::eColor;
        if ((aspect & ImageAspectFlags::Depth) != ImageAspectFlags::None)
            result |= vk::ImageAspectFlagBits::eDepth;
        if ((aspect & ImageAspectFlags::Stencil) != ImageAspectFlags::None)
            result |= vk::ImageAspectFlagBits::eStencil;
        return result;
    }

    inline vk::ImageSubresourceRange VKConvert(const ImageSubresourceRange& range)
    {
        vk::ImageSubresourceRange result{};
        result.baseArrayLayer = range.MinArraySlice;
        result.layerCount     = range.ArraySliceCount;
        result.baseMipLevel   = range.MinMipSlice;
        result.levelCount     = range.MipSliceCount;
        result.aspectMask     = VKConvert(range.Apsect);
        return result;
    }

    inline vk::AccessFlags GetSrcAccessMask(ResourceState state)
    {
        static auto Conversions = []() {
            std::array<vk::AccessFlags, static_cast<size_t>(Format::Count)> result{};
            using Bits = vk::AccessFlagBits;
#define FE_CVT_ENTRY(state) result[static_cast<size_t>(ResourceState::state)]
            FE_CVT_ENTRY(None)             = static_cast<Bits>(0);
            FE_CVT_ENTRY(Common)           = static_cast<Bits>(0);
            FE_CVT_ENTRY(VertexBuffer)     = Bits::eVertexAttributeRead;
            FE_CVT_ENTRY(ConstantBuffer)   = Bits::eUniformRead;
            FE_CVT_ENTRY(IndexBuffer)      = Bits::eIndexRead;
            FE_CVT_ENTRY(RenderTarget)     = Bits::eColorAttachmentRead | Bits::eColorAttachmentWrite;
            FE_CVT_ENTRY(UnorderedAccess)  = Bits::eShaderRead | Bits::eShaderWrite;
            FE_CVT_ENTRY(DepthWrite)       = Bits::eDepthStencilAttachmentRead | Bits::eDepthStencilAttachmentWrite;
            FE_CVT_ENTRY(DepthRead)        = Bits::eDepthStencilAttachmentRead;
            FE_CVT_ENTRY(ShaderResource)   = Bits::eShaderRead;
            FE_CVT_ENTRY(IndirectArgument) = Bits::eIndirectCommandRead;
            FE_CVT_ENTRY(CopyDest)         = Bits::eTransferWrite;
            FE_CVT_ENTRY(CopySource)       = Bits::eTransferRead;
            FE_CVT_ENTRY(Present)          = Bits::eMemoryRead;
#undef FE_CVT_ENTRY
            return result;
        }();

        return Conversions[static_cast<size_t>(state)];
    }

    VKCommandBuffer::VKCommandBuffer(VKDevice& dev, CommandQueueClass cmdQueueClass)
        : m_Device(&dev)
        , m_IsUpdating(false)
    {
        auto& nativeDevice = m_Device->GetNativeDevice();
        vk::CommandBufferAllocateInfo allocateInfo{};
        allocateInfo.commandPool        = m_Device->GetCommandPool(cmdQueueClass);
        allocateInfo.commandBufferCount = 1;
        allocateInfo.level              = vk::CommandBufferLevel::ePrimary;

        // Only to reduce length of the next line
        using Allocator = StdHeapAllocator<vk::UniqueCommandBuffer>;
        auto buffers    = nativeDevice.allocateCommandBuffersUnique<VULKAN_HPP_DEFAULT_DISPATCHER_TYPE, Allocator>(allocateInfo);
        m_CommandBuffer = std::move(buffers.front());
    }

    vk::CommandBuffer& VKCommandBuffer::GetNativeBuffer()
    {
        return m_CommandBuffer.get();
    }

    void VKCommandBuffer::Begin()
    {
        m_CommandBuffer->begin(vk::CommandBufferBeginInfo{});
    }

    void VKCommandBuffer::End()
    {
        m_CommandBuffer->end();
    }

    void VKCommandBuffer::Draw(UInt32 vertexCount, UInt32 instanceCount, UInt32 firstVertex, UInt32 firstInstance)
    {
        m_CommandBuffer->draw(vertexCount, instanceCount, firstVertex, firstInstance);
    }

    void VKCommandBuffer::DrawIndexed(
        UInt32 indexCount, UInt32 instanceCount, UInt32 firstIndex, Int32 vertexOffset, UInt32 firstInstance)
    {
        m_CommandBuffer->drawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }

    void VKCommandBuffer::SetViewport(const Viewport& viewport)
    {
        vk::Viewport vp{};
        vp.x        = viewport.MinX;
        vp.y        = viewport.MinY;
        vp.width    = viewport.Width();
        vp.height   = viewport.Height();
        vp.minDepth = viewport.MinZ;
        vp.maxDepth = viewport.MaxZ;
        m_CommandBuffer->setViewport(0, 1, &vp);
    }

    void VKCommandBuffer::SetScissor(const Scissor& scissor)
    {
        vk::Rect2D rect{};
        rect.offset = vk::Offset2D(scissor.MinX, scissor.MaxX);
        rect.extent = vk::Extent2D(scissor.Width(), scissor.Height());
        m_CommandBuffer->setScissor(0, 1, &rect);
    }

    void VKCommandBuffer::ResourceTransitionBarriers(const Vector<ResourceTransitionBarrierDesc>& barriers)
    {
        Vector<vk::ImageMemoryBarrier> nativeBarriers;
        for (auto& barrier : barriers)
        {
            VKImage* img = static_cast<VKImage*>(barrier.Image);

            auto before = VKConvert(barrier.StateBefore);
            auto after  = VKConvert(barrier.StateAfter);

            if (before == after)
                continue;

            vk::ImageMemoryBarrier& imageMemoryBarrier = nativeBarriers.emplace_back();
            imageMemoryBarrier.oldLayout               = before;
            imageMemoryBarrier.newLayout               = after;
            imageMemoryBarrier.srcQueueFamilyIndex     = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.dstQueueFamilyIndex     = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.image                   = img->Image;
            imageMemoryBarrier.subresourceRange        = VKConvert(barrier.SubresourceRange);

            imageMemoryBarrier.srcAccessMask = GetSrcAccessMask(barrier.StateBefore);
            imageMemoryBarrier.dstAccessMask = GetSrcAccessMask(barrier.StateAfter);

            if (after == vk::ImageLayout::eShaderReadOnlyOptimal)
            {
                imageMemoryBarrier.srcAccessMask |= vk::AccessFlagBits::eHostWrite;
                imageMemoryBarrier.srcAccessMask |= vk::AccessFlagBits::eTransferWrite;
            }
        }

        if (nativeBarriers.empty())
            return;

        m_CommandBuffer->pipelineBarrier(
            vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, vk::DependencyFlagBits::eByRegion,
            0, nullptr, 0, nullptr, static_cast<UInt32>(nativeBarriers.size()), nativeBarriers.data());
    }

    void VKCommandBuffer::MemoryBarrier()
    {
        vk::MemoryBarrier barrier{};
        barrier.dstAccessMask = barrier.srcAccessMask = vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead;
        m_CommandBuffer->pipelineBarrier(
            vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, vk::DependencyFlagBits::eByRegion,
            1, &barrier, 0, nullptr, 0, nullptr);
    }
} // namespace FE::GPU
