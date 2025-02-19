#pragma once
#include <Graphics/RHI/FrameGraph/FrameGraph.h>
#include <Graphics/RHI/Vulkan/Base/Config.h>

namespace FE::Graphics::Vulkan
{
    struct Device;

    struct BufferBarrierDesc final
    {
        VkBuffer m_buffer;
        uint32_t m_sourceAccess : 16;
        uint32_t m_destAccess : 16;
        RHI::HardwareQueueKindFlags m_sourceQueueKind : 16;
        RHI::HardwareQueueKindFlags m_destQueueKind : 16;

        BufferBarrierDesc()
            : m_buffer(VK_NULL_HANDLE)
            , m_sourceAccess(0)
            , m_destAccess(0)
            , m_sourceQueueKind(RHI::HardwareQueueKindFlags::kNone)
            , m_destQueueKind(RHI::HardwareQueueKindFlags::kNone)
        {
        }

        [[nodiscard]] uint64_t GetHash() const
        {
            return DefaultHash(this, sizeof(*this));
        }
    };


    struct ImageBarrierDesc final
    {
        VkImage m_image;
        uint32_t m_sourceAccess : 16;
        uint32_t m_destAccess : 16;
        RHI::HardwareQueueKindFlags m_sourceQueueKind : 16;
        RHI::HardwareQueueKindFlags m_destQueueKind : 16;

        ImageBarrierDesc()
            : m_image(VK_NULL_HANDLE)
            , m_sourceAccess(0)
            , m_destAccess(0)
            , m_sourceQueueKind(RHI::HardwareQueueKindFlags::kNone)
            , m_destQueueKind(RHI::HardwareQueueKindFlags::kNone)
        {
        }

        [[nodiscard]] uint64_t GetHash() const
        {
            return DefaultHash(this, sizeof(*this));
        }
    };


    struct ResourceBarrierBatcher final
    {
        ResourceBarrierBatcher(Device* device)
            : m_device(device)
        {
        }

        void AddBarrier(const BufferBarrierDesc& desc);
        void AddBarrier(const ImageBarrierDesc& desc);

        void Flush(VkCommandBuffer commandBuffer);

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

        festd::small_vector<BufferBarrierWithHash> m_bufferBarriers;
        festd::small_vector<ImageBarrierWithHash> m_imageBarriers;
    };
} // namespace FE::Graphics::Vulkan
