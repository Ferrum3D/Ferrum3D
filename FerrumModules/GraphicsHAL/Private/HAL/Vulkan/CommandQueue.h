#pragma once
#include <HAL/CommandQueue.h>
#include <HAL/Vulkan/Common/Config.h>

namespace FE::Graphics::Vulkan
{
    class Device;

    struct CommandQueueDesc
    {
        uint32_t QueueFamilyIndex;
        uint32_t QueueIndex;
    };


    class CommandQueue : public HAL::CommandQueue
    {
        VkQueue m_Queue = VK_NULL_HANDLE;
        CommandQueueDesc m_Desc;

    public:
        FE_RTTI_Class(CommandQueue, "416B9666-BFB4-4DB6-85C8-1AB6D5A318C5");

        CommandQueue(HAL::Device* pDevice, const CommandQueueDesc& desc);
        ~CommandQueue() override = default;

        [[nodiscard]] inline VkQueue GetNative() const
        {
            return m_Queue;
        }

        void SignalFence(HAL::Fence* fence) override;
        void SubmitBuffers(festd::span<HAL::CommandList*> commandLists, HAL::Fence* signalFence, HAL::SubmitFlags flags) override;

        [[nodiscard]] const CommandQueueDesc& GetDesc() const;
    };

    FE_ENABLE_IMPL_CAST(CommandQueue);
} // namespace FE::Graphics::Vulkan
