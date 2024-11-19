#pragma once
#include <Graphics/RHI/CommandList.h>
#include <Graphics/RHI/Vulkan/Common/Config.h>

namespace FE::Graphics::Vulkan
{
    struct CommandList : public RHI::CommandList
    {
        FE_RTTI_Class(CommandList, "9A422C8F-72E8-48DD-8BE6-63D059AE9432");

        CommandList(RHI::Device* device);
        ~CommandList() override;

        RHI::ResultCode Init(const RHI::CommandListDesc& desc) override;

        [[nodiscard]] VkCommandBuffer GetNative() const
        {
            return m_commandBuffer;
        }

        void Begin() override;
        void End() override;

        void BindShaderResourceGroups(festd::span<RHI::ShaderResourceGroup* const> shaderResourceGroups,
                                      RHI::GraphicsPipeline* pipeline) override;
        void BindGraphicsPipeline(RHI::GraphicsPipeline* pipeline) override;

        void BeginRenderPass(RHI::RenderPass* renderPass, RHI::Framebuffer* framebuffer,
                             festd::span<const RHI::ClearValueDesc> clearValues) override;
        void EndRenderPass() override;

        void BindIndexBuffer(RHI::Buffer* buffer, uint64_t byteOffset) override;
        void BindVertexBuffer(uint32_t slot, RHI::Buffer* buffer, uint64_t byteOffset) override;
        void BindVertexBuffers(uint32_t startSlot, festd::span<RHI::Buffer* const> buffers,
                               festd::span<const uint64_t> offsets) override;

        void CopyBuffers(RHI::Buffer* source, RHI::Buffer* dest, const RHI::BufferCopyRegion& region) override;
        void CopyBufferToImage(RHI::Buffer* source, RHI::Image* dest, const RHI::BufferImageCopyRegion& region) override;

        void BlitImage(RHI::Image* source, RHI::Image* dest, const RHI::ImageBlitRegion& region) override;

        void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) override;
        void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset,
                         uint32_t firstInstance) override;

        void SetViewport(RHI::Viewport viewport) override;
        void SetScissor(RHI::Scissor scissor) override;

        virtual void ResourceTransitionBarriers(festd::span<const RHI::ImageBarrierDesc> imageBarriers,
                                                festd::span<const RHI::BufferBarrierDesc> bufferBarriers) override;
        void MemoryBarrier() override;

    private:
        VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
        VkCommandPool m_commandPool = VK_NULL_HANDLE;
        bool m_isUpdating;
    };

    FE_ENABLE_NATIVE_CAST(CommandList);
} // namespace FE::Graphics::Vulkan
