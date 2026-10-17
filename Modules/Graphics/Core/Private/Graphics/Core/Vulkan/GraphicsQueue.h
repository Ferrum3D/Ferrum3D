#pragma once
#include <Core/Memory/PoolAllocator.h>
#include <Core/Threading/SpinLock.h>
#include <Graphics/Core/GraphicsQueue.h>
#include <Graphics/Core/Vulkan/CommandBuffer.h>
#include <Graphics/Core/Vulkan/Fence.h>
#include <Graphics/Core/Vulkan/GpuProfiler.h>
#include <festd/vector.h>

namespace FE::Graphics::Vulkan
{
    struct GraphicsQueue final : public Core::GraphicsQueue
    {
        FE_RTTI("3830A626-8EEE-4FFE-8F17-0195DDE01262");

        explicit GraphicsQueue(Core::Device* device);
        ~GraphicsQueue() override;

        [[nodiscard]] GpuProfiler& GetGpuProfiler()
        {
            return m_gpuProfiler;
        }


        CommandBuffer* GetCurrentCommandBuffer();

        [[nodiscard]] uint64_t GetFrameIndex() const
        {
            return m_frameIndex;
        }

        [[nodiscard]] VkQueue GetNative() const
        {
            return m_nativeQueue;
        }

        [[nodiscard]] uint64_t GetTrackedFenceValue() const
        {
            return m_trackedFenceValue.load();
        }

        [[nodiscard]] uint64_t GetCompletedFenceValue() const
        {
            return m_fence->GetCompletedValue();
        }

        Core::FenceSyncPoint GetCurrentFence() const override;

        void PublishShaderRead(Core::Buffer* buffer) override;
        void PublishShaderRead(Core::Texture* texture) override;

        void BeginFrame() override;
        void SubmitFrame() override;
        Core::FenceSyncPoint CloseFrame() override;
        void Drain() override;

    private:
        void BeginRecording();

        void DestroyObject() override
        {
            Memory::DefaultDelete(this);
        }

        std::atomic<uint64_t> m_frameIndex = 1;
        std::atomic<uint64_t> m_trackedFenceValue = 0;
        Rc<Fence> m_fence;

        bool m_isActive = false;
        bool m_isRecording = false;

        VkQueue m_nativeQueue = VK_NULL_HANDLE;
        GpuProfiler m_gpuProfiler;

        Memory::SpinLockedPoolAllocator m_sharedPagePool{ "Graphics/GraphicsCommandBufferPagePool", 8096 };
        festd::inline_vector<Rc<CommandBuffer>> m_graphicsCommandBuffers;
        Threading::SpinLock m_publicationLock;
        festd::vector<Rc<Core::Buffer>> m_frameShaderReadBuffers;
        festd::vector<Rc<Core::Texture>> m_frameShaderReadTextures;
        festd::vector<Rc<Core::Buffer>> m_pendingShaderReadBuffers;
        festd::vector<Rc<Core::Texture>> m_pendingShaderReadTextures;
    };

    FE_ENABLE_IMPL_CAST(GraphicsQueue);
} // namespace FE::Graphics::Vulkan
