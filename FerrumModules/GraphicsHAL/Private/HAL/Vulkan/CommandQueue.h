#pragma once
#include <HAL/CommandQueue.h>
#include <HAL/Vulkan/Common/Config.h>

namespace FE::Graphics::Vulkan
{
    class Device;

    struct CommandQueueDesc final
    {
        uint32_t m_queueFamilyIndex;
        uint32_t m_queueIndex;
    };


    struct CommandQueue final : public HAL::CommandQueue
    {
        FE_RTTI_Class(CommandQueue, "416B9666-BFB4-4DB6-85C8-1AB6D5A318C5");

        CommandQueue(HAL::Device* pDevice, CommandQueueDesc desc);
        ~CommandQueue() override = default;

        [[nodiscard]] inline VkQueue GetNative() const
        {
            return m_queue;
        }

        void SignalFence(const HAL::FenceSyncPoint& fence) override;
        void WaitFence(const HAL::FenceSyncPoint& fence) override;

        void Execute(festd::span<HAL::CommandList* const> commandLists) override;

        [[nodiscard]] const CommandQueueDesc& GetDesc() const;

    private:
        VkQueue m_queue = VK_NULL_HANDLE;
        CommandQueueDesc m_desc;
    };

    FE_ENABLE_NATIVE_CAST(CommandQueue);
} // namespace FE::Graphics::Vulkan
