#pragma once
#include <Graphics/Core/FrameGraph/Base.h>
#include <Graphics/Core/ImageBase.h>
#include <Graphics/Core/Vulkan/Base/Config.h>
#include <Graphics/Core/Vulkan/Device.h>

namespace FE::Graphics::Vulkan
{
    struct Device;

    struct BufferBarrierDesc final
    {
        VkBuffer m_buffer = VK_NULL_HANDLE;
        Core::BufferAccessType m_sourceAccess : 16;
        Core::BufferAccessType m_destAccess : 16;
        Core::HardwareQueueKindFlags m_sourceQueueKind : 16;
        Core::HardwareQueueKindFlags m_destQueueKind : 16;

        BufferBarrierDesc()
            : m_sourceAccess(Core::BufferAccessType::kUndefined)
            , m_destAccess(Core::BufferAccessType::kUndefined)
            , m_sourceQueueKind(Core::HardwareQueueKindFlags::kNone)
            , m_destQueueKind(Core::HardwareQueueKindFlags::kNone)
        {
        }

        [[nodiscard]] uint64_t GetHash() const
        {
            return DefaultHash(this, sizeof(*this));
        }
    };


    struct ImageBarrierDesc final
    {
        VkImage m_image = VK_NULL_HANDLE;
        Core::ImageAccessType m_sourceAccess : 16;
        Core::ImageAccessType m_destAccess : 16;
        Core::HardwareQueueKindFlags m_sourceQueueKind : 16;
        Core::HardwareQueueKindFlags m_destQueueKind : 16;
        Core::ImageSubresource m_subresource = {};
        Core::Format m_format = Core::Format::kUndefined;

        ImageBarrierDesc()
            : m_sourceAccess(Core::ImageAccessType::kUndefined)
            , m_destAccess(Core::ImageAccessType::kUndefined)
            , m_sourceQueueKind(Core::HardwareQueueKindFlags::kNone)
            , m_destQueueKind(Core::HardwareQueueKindFlags::kNone)
        {
        }

        [[nodiscard]] uint64_t GetHash() const
        {
            return DefaultHash(this, sizeof(*this));
        }
    };


    struct ResourceBarrierBatcher final
    {
        ResourceBarrierBatcher(Core::Device* device)
            : m_device(ImplCast(device))
        {
        }

        void Begin(VkCommandBuffer commandBuffer);

        void AddBarrier(const BufferBarrierDesc& desc);
        void AddBarrier(const ImageBarrierDesc& desc);

        void Flush();

    private:
        struct BufferBarrierWithHash final
        {
            BufferBarrierDesc m_desc;
            uint64_t m_hash = 0;
        };

        struct ImageBarrierWithHash final
        {
            ImageBarrierDesc m_desc;
            uint64_t m_hash = 0;
        };

        Device* m_device = nullptr;
        VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;

        festd::inline_vector<BufferBarrierWithHash> m_bufferBarriers;
        festd::inline_vector<ImageBarrierWithHash> m_imageBarriers;
    };
} // namespace FE::Graphics::Vulkan
