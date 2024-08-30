#pragma once
#include <HAL/CommandList.h>
#include <HAL/Vulkan/Common/Config.h>

namespace FE::Graphics::Vulkan
{
    class CommandList : public HAL::CommandList
    {
        VkCommandBuffer m_CommandBuffer = VK_NULL_HANDLE;
        VkCommandPool m_CommandPool = VK_NULL_HANDLE;
        bool m_IsUpdating;

    public:
        FE_RTTI_Class(CommandList, "9A422C8F-72E8-48DD-8BE6-63D059AE9432");

        CommandList(HAL::Device* pDevice);
        ~CommandList() override;

        HAL::ResultCode Init(const HAL::CommandListDesc& desc) override;

        [[nodiscard]] inline VkCommandBuffer GetNative() const
        {
            return m_CommandBuffer;
        }

        void Begin() override;
        void End() override;

        void BindShaderResourceGroups(festd::span<HAL::ShaderResourceGroup*> shaderResourceGroups,
                                      HAL::GraphicsPipeline* pipeline) override;
        void BindGraphicsPipeline(HAL::GraphicsPipeline* pipeline) override;

        void BeginRenderPass(HAL::RenderPass* renderPass, HAL::Framebuffer* framebuffer,
                             const festd::span<HAL::ClearValueDesc>& clearValues) override;
        void EndRenderPass() override;

        void BindIndexBuffer(HAL::Buffer* buffer, uint64_t byteOffset) override;
        void BindVertexBuffer(uint32_t slot, HAL::Buffer* buffer, uint64_t byteOffset) override;
        void BindVertexBuffers(uint32_t startSlot, festd::span<HAL::Buffer*> buffers,
                               festd::span<const uint64_t> offsets) override;

        void CopyBuffers(HAL::Buffer* source, HAL::Buffer* dest, const HAL::BufferCopyRegion& region) override;
        void CopyBufferToImage(HAL::Buffer* source, HAL::Image* dest, const HAL::BufferImageCopyRegion& region) override;

        void BlitImage(HAL::Image* source, HAL::Image* dest, const HAL::ImageBlitRegion& region) override;

        void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) override;
        void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset,
                         uint32_t firstInstance) override;

        void SetViewport(const HAL::Viewport& viewport) override;
        void SetScissor(const HAL::Scissor& scissor) override;

        virtual void ResourceTransitionBarriers(festd::span<const HAL::ImageBarrierDesc> imageBarriers,
                                                festd::span<const HAL::BufferBarrierDesc> bufferBarriers) override;
        void MemoryBarrier() override;
    };

    FE_ENABLE_NATIVE_CAST(CommandList);
} // namespace FE::Graphics::Vulkan
