#pragma once
#include <FeCore/Memory/PoolAllocator.h>
#include <Graphics/Core/Vulkan/CommandBuffer.h>
#include <Graphics/Core/Vulkan/Fence.h>
#include <festd/vector.h>

namespace FE::Graphics::Vulkan
{
    struct GraphicsCommandQueue final : public Core::DeviceObject
    {
        GraphicsCommandQueue(Core::Device* device);

        FE_RTTI_Class(GraphicsCommandQueue, "3830A626-8EEE-4FFE-8F17-0195DDE01262");

        CommandBuffer* GetCurrentGraphicsCommandBuffer();

        [[nodiscard]] uint64_t GetFrameIndex() const
        {
            return m_frameIndex;
        }

        [[nodiscard]] VkQueue GetNative() const
        {
            return m_nativeQueue;
        }

        void WaitForPreviousFrame();
        Core::FenceSyncPoint CloseFrame();

    private:
        uint64_t m_frameIndex = 1;
        Rc<Fence> m_fence;

        VkQueue m_nativeQueue = VK_NULL_HANDLE;

        Memory::SpinLockedPoolAllocator m_sharedPagePool{ "GraphicsCommandBufferPagePool", 8096 };
        festd::inline_vector<Rc<CommandBuffer>> m_graphicsCommandBuffers;
    };
} // namespace FE::Graphics::Vulkan
