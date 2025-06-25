#pragma once
#include <FeCore/Containers/SegmentedVector.h>
#include <FeCore/Memory/LinearAllocator.h>
#include <Graphics/Core/Fence.h>
#include <Graphics/Core/Vulkan/Base/BaseTypes.h>
#include <Graphics/Core/Vulkan/ResourceBarrierBatcher.h>

namespace FE::Graphics::Vulkan
{
    struct CommandBufferDesc final
    {
        VkCommandBufferLevel m_level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        Env::Name m_name;
        VkQueue m_queue = VK_NULL_HANDLE;
        VkCommandPool m_commandPool = VK_NULL_HANDLE;
        std::pmr::memory_resource* m_pageAllocator = nullptr;
    };


    struct CommandBuffer final : public Core::DeviceObject
    {
        FE_RTTI_Class(CommandBuffer, "F14E4F22-6734-4747-BB3C-FE3BA6697E46");

        ~CommandBuffer() override;

        static CommandBuffer* Create(Core::Device* device, const CommandBufferDesc& desc);

        [[nodiscard]] VkCommandBuffer GetNative() const
        {
            return m_nativeCommandBuffer;
        }

        void EnqueueFenceToWait(const Core::FenceSyncPoint& fence)
        {
            FE_Assert(fence.m_fence);
            m_waitFences.push_back(fence);
        }

        void EnqueueFenceToSignal(const Core::FenceSyncPoint& fence)
        {
            FE_Assert(fence.m_fence);
            m_signalFences.push_back(fence);
        }

        void EnqueueSemaphoreToWait(Semaphore* semaphore, const VkPipelineStageFlags stageMask)
        {
            FE_Assert(semaphore && semaphore->GetNative());
            m_waitSemaphores.push_back({ semaphore, stageMask });
        }

        void EnqueueSemaphoreToSignal(Semaphore* semaphore)
        {
            FE_Assert(semaphore && semaphore->GetNative());
            m_signalSemaphores.push_back(semaphore);
        }

        void Begin();
        void Submit();

    private:
        explicit CommandBuffer(Core::Device* device, const CommandBufferDesc& desc);

        Memory::LinearAllocator m_linearAllocator;

        VkQueue m_nativeQueue = VK_NULL_HANDLE;
        VkCommandBuffer m_nativeCommandBuffer = VK_NULL_HANDLE;
        VkCommandPool m_nativeCommandPool = VK_NULL_HANDLE;

        struct WaitSemaphore final
        {
            Rc<Semaphore> m_semaphore;
            VkPipelineStageFlags m_stageMask;
        };

        bool m_wasUsed = false;

        ResourceBarrierBatcher m_resourceBarrierBatcher;
        SegmentedVector<Rc<Semaphore>, 256> m_signalSemaphores;
        SegmentedVector<WaitSemaphore, 512> m_waitSemaphores;

        SegmentedVector<Core::FenceSyncPoint, 256> m_signalFences;
        SegmentedVector<Core::FenceSyncPoint, 256> m_waitFences;
    };
} // namespace FE::Graphics::Vulkan
