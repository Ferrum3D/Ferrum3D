#pragma once
#include <GPU/CommandBuffer/ICommandBuffer.h>
#include <GPU/Common/VKConfig.h>
#include <GPU/Device/IDevice.h>

namespace FE::GPU
{
    class VKDevice;

    inline vk::Viewport VKConvert(const Viewport& viewport)
    {
        vk::Viewport vp{};
        vp.x        = viewport.MinX;
        vp.y        = viewport.MinY;
        vp.width    = viewport.Width();
        vp.height   = viewport.Height();
        vp.minDepth = viewport.MinZ;
        vp.maxDepth = viewport.MaxZ;
        return vp;
    }

    inline vk::Rect2D VKConvert(const Scissor& scissor)
    {
        vk::Rect2D rect{};
        rect.offset = vk::Offset2D(scissor.MinX, scissor.MinY);
        rect.extent = vk::Extent2D(scissor.Width(), scissor.Height());
        return rect;
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
        result.aspectMask     = VKConvert(range.AspectFlags);
        return result;
    }

    inline vk::AccessFlags GetAccessMask(ResourceState state)
    {
        static auto Conversions = []() {
            std::array<vk::AccessFlags, static_cast<size_t>(Format::Count)> result{};
            using Bits = vk::AccessFlagBits;
#define FE_CVT_ENTRY(state) result[static_cast<size_t>(ResourceState::state)]
            FE_CVT_ENTRY(Undefined)        = static_cast<Bits>(0);
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

    inline vk::ImageLayout VKConvert(ResourceState state)
    {
        static auto Conversions = []() {
            std::array<vk::ImageLayout, static_cast<size_t>(Format::Count)> result{};
            result[static_cast<size_t>(ResourceState::Undefined)]       = vk::ImageLayout::eUndefined;
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

    class VKCommandBuffer : public Object<ICommandBuffer>
    {
        VKDevice* m_Device;
        vk::UniqueCommandBuffer m_CommandBuffer;
        bool m_IsUpdating;

    public:
        FE_CLASS_RTTI(VKCommandBuffer, "9A422C8F-72E8-48DD-8BE6-63D059AE9432");

        VKCommandBuffer(VKDevice& dev, CommandQueueClass cmdQueueClass);
        VKCommandBuffer(VKDevice& dev, UInt32 queueFamilyIndex);

        vk::CommandBuffer& GetNativeBuffer();

        void Begin() override;
        void End() override;

        void BindDescriptorTables(const List<IDescriptorTable*>& descriptorTables, IGraphicsPipeline* pipeline) override;
        void BindGraphicsPipeline(IGraphicsPipeline* pipeline) override;

        void BeginRenderPass(IRenderPass* renderPass, IFramebuffer* framebuffer, const ClearValueDesc& clearValue) override;
        void EndRenderPass() override;

        void BindVertexBuffer(UInt32 slot, IBuffer* buffer) override;
        void BindIndexBuffer(IBuffer* buffer) override;

        void CopyBuffers(IBuffer* source, IBuffer* dest, const BufferCopyRegion& region) override;

        void Draw(UInt32 vertexCount, UInt32 instanceCount, UInt32 firstVertex, UInt32 firstInstance) override;
        void DrawIndexed(
            UInt32 indexCount, UInt32 instanceCount, UInt32 firstIndex, Int32 vertexOffset, UInt32 firstInstance) override;

        void SetViewport(const Viewport& viewport) override;
        void SetScissor(const Scissor& scissor) override;

        void ResourceTransitionBarriers(const List<ResourceTransitionBarrierDesc>& barriers) override;
        void MemoryBarrier() override;
    };
} // namespace FE::GPU
