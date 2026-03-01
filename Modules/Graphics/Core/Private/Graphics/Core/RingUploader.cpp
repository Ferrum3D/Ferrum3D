#include <Graphics/Core/FrameGraph/FrameGraph.h>
#include <Graphics/Core/ResourcePool.h>
#include <Graphics/Core/RingUploader.h>

namespace FE::Graphics::Core
{
    RingUploader::~RingUploader()
    {
        Shutdown();
    }


    void RingUploader::Setup(const Env::Name name, ResourcePool* resourcePool, const uint32_t capacity)
    {
        FE_Assert(m_buffer == nullptr);
        m_buffer = resourcePool->CreateByteAddressBuffer(name, capacity);

        BufferCommitParams commitParams;
        commitParams.m_bindFlags = BarrierAccessFlags::kCopySourceAndDest;
        commitParams.m_memory = ResourceMemory::kHostWriteThrough;
        resourcePool->CommitBufferMemory(m_buffer.Get(), commitParams);
        m_mappedMemory = static_cast<std::byte*>(m_buffer->Map());
    }


    void RingUploader::Shutdown()
    {
        if (m_buffer != nullptr)
        {
            FE_Assert(m_mappedMemory != nullptr);

            CheckPendingFrames();
            FE_Assert(m_pendingUploads.empty());

            m_buffer->Unmap();
            m_buffer.Reset();
        }
    }


    bool RingUploader::Upload(FrameGraph& graph, const BufferView destination, const void* source, const uint32_t byteSize)
    {
        if (const auto allocation = m_ringBuffer.Allocate(byteSize, kAlignment))
        {
            // TODO: We don't necessarily want a contiguous block of memory here. It is possible to save some memory by splitting
            //       copy operations in two when the ring buffer has to wrap around.

            void* hostDestination = m_mappedMemory + allocation.m_offset;
            memcpy(hostDestination, source, byteSize);

            const BufferView deviceDestination = destination.Slice(byteSize);
            const BufferView deviceSource{ m_buffer.Get(), BufferSlice{ allocation.m_offset, byteSize } };
            graph.AddCopyPass(deviceDestination, deviceSource);

            m_currentFrameBytes += allocation.m_size;

            return true;
        }

        return false;
    }


    void RingUploader::CloseFrame(const FenceSyncPoint& fence)
    {
        CheckPendingFrames();

        if (m_currentFrameBytes == 0)
            return;

        FrameData& frameData = m_pendingUploads.push_back();
        frameData.m_fence = fence;
        frameData.m_bytesAllocated = m_currentFrameBytes;
    }


    void RingUploader::CheckPendingFrames()
    {
        while (!m_pendingUploads.empty())
        {
            FrameData& frameData = m_pendingUploads.front();
            if (!frameData.m_fence.IsReady())
                break;

            m_ringBuffer.Free(frameData.m_bytesAllocated);
            m_pendingUploads.pop_front();
        }
    }
} // namespace FE::Graphics::Core
