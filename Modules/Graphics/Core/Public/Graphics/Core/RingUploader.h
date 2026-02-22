#pragma once
#include <FeCore/Memory/RefCount.h>
#include <FeCore/Memory/RingBufferAllocator.h>
#include <Graphics/Core/Buffer.h>
#include <Graphics/Core/Fence.h>
#include <festd/ring_buffer.h>

namespace FE::Graphics::Core
{
    struct RingUploader final
    {
        ~RingUploader();

        void Setup(Env::Name name, ResourcePool* resourcePool, uint32_t capacity);
        void Shutdown();

        [[nodiscard]] bool Upload(FrameGraph& graph, BufferView destination, const void* source, uint32_t byteSize);
        void CloseFrame(const FenceSyncPoint& fence);

    private:
        static constexpr uint32_t kAlignment = 256;

        struct FrameData final
        {
            FenceSyncPoint m_fence;
            uint32_t m_bytesAllocated = 0;
        };

        void CheckPendingFrames();

        uint32_t m_currentFrameBytes = 0;

        Rc<Buffer> m_buffer;
        std::byte* m_mappedMemory = nullptr;

        Memory::RingBufferAllocator m_ringBuffer;
        festd::ring_buffer<FrameData> m_pendingUploads;
    };
} // namespace FE::Graphics::Core
