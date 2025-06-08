#pragma once
#include <Graphics/Core/Vulkan/Base/Config.h>
#include <Graphics/Core/Vulkan/Device.h>

namespace FE::Graphics::Vulkan
{
    struct Device;

    struct BufferBarrierDesc final
    {
        VkBuffer m_buffer = VK_NULL_HANDLE;
        uint32_t m_sourceAccess : 16;
        uint32_t m_destAccess : 16;
        Core::HardwareQueueKindFlags m_sourceQueueKind : 16;
        Core::HardwareQueueKindFlags m_destQueueKind : 16;
        uint32_t m_offset = 0;
        uint32_t m_size = 0;

        BufferBarrierDesc()
            : m_sourceAccess(0)
            , m_destAccess(0)
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
        VkImage m_image;
        uint32_t m_sourceAccess : 16;
        uint32_t m_destAccess : 16;
        Core::HardwareQueueKindFlags m_sourceQueueKind : 16;
        Core::HardwareQueueKindFlags m_destQueueKind : 16;

        ImageBarrierDesc()
            : m_image(VK_NULL_HANDLE)
            , m_sourceAccess(0)
            , m_destAccess(0)
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

        festd::small_vector<BufferBarrierWithHash> m_bufferBarriers;
        festd::small_vector<ImageBarrierWithHash> m_imageBarriers;
    };
} // namespace FE::Graphics::Vulkan
