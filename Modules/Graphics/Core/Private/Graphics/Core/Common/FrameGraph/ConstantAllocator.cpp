#include <Graphics/Core/Common/FrameGraph/ConstantAllocator.h>
#include <Graphics/Core/ResourcePool.h>

namespace FE::Graphics::Common
{
    ConstantAllocator::ConstantAllocator(Core::ResourcePool* resourcePool)
        : m_resourcePool(resourcePool)
    {
    }


    ConstantAllocator::~ConstantAllocator()
    {
        const auto unmap = [](festd::vector<Chunk>& chunks) {
            for (Chunk& chunk : chunks)
                chunk.m_buffer->Unmap();
        };

        unmap(m_currentChunks);
        unmap(m_pendingChunks);
        unmap(m_freeChunks);
    }


    ConstantAllocator::Allocation ConstantAllocator::Allocate(const uint32_t byteSize)
    {
        FE_Assert(byteSize > 0);

        const uint32_t alignedSize = AlignUp(byteSize, kAlignment);
        Chunk* chunk = m_currentChunks.empty() ? nullptr : &m_currentChunks.back();
        if (chunk == nullptr || chunk->m_capacity - chunk->m_used < alignedSize)
            chunk = &AcquireChunk(alignedSize);

        const uint32_t offset = chunk->m_used;
        chunk->m_used += alignedSize;
        return { chunk->m_hostPointer + offset, chunk->m_deviceAddress + offset };
    }


    bool ConstantAllocator::IsCurrentFrameAddress(const uint64_t deviceAddress) const
    {
        for (const Chunk& chunk : m_currentChunks)
        {
            if (deviceAddress >= chunk.m_deviceAddress && deviceAddress < chunk.m_deviceAddress + chunk.m_used)
                return true;
        }

        return false;
    }


    void ConstantAllocator::BeginFrame()
    {
        // Fences complete in submission order.
        uint32_t readyCount = 0;
        while (readyCount < m_pendingChunks.size() && m_pendingChunks[readyCount].m_fence.IsReady())
            ++readyCount;

        for (uint32_t chunkIndex = 0; chunkIndex < readyCount; ++chunkIndex)
        {
            m_pendingChunks[chunkIndex].m_used = 0;
            m_freeChunks.push_back(std::move(m_pendingChunks[chunkIndex]));
        }

        m_pendingChunks.erase(m_pendingChunks.begin(), m_pendingChunks.begin() + readyCount);
    }


    void ConstantAllocator::EndFrame(const Core::FenceSyncPoint& fence)
    {
        for (Chunk& chunk : m_currentChunks)
        {
            chunk.m_buffer->FlushMappedRange(0, chunk.m_used);
            chunk.m_fence = fence;
            m_pendingChunks.push_back(std::move(chunk));
        }

        m_currentChunks.clear();
    }


    ConstantAllocator::Chunk& ConstantAllocator::AcquireChunk(const uint32_t byteSize)
    {
        for (uint32_t chunkIndex = 0; chunkIndex < m_freeChunks.size(); ++chunkIndex)
        {
            if (m_freeChunks[chunkIndex].m_capacity < byteSize)
                continue;

            m_currentChunks.push_back(std::move(m_freeChunks[chunkIndex]));
            m_freeChunks.erase(m_freeChunks.begin() + chunkIndex);
            return m_currentChunks.back();
        }

        m_currentChunks.push_back(CreateChunk(Math::Max(byteSize, kChunkByteSize)));
        return m_currentChunks.back();
    }


    ConstantAllocator::Chunk ConstantAllocator::CreateChunk(const uint32_t capacity) const
    {
        Chunk chunk;
        chunk.m_capacity = capacity;
        chunk.m_buffer = Core::Buffer::CreateByteAddress(m_resourcePool->GetDevice(), "FrameGraphConstants", capacity);

        Core::ResourceCommitParams commitParams;
        commitParams.m_memory = Core::ResourceMemory::kHostWriteThrough;
        commitParams.m_bindFlags = Core::BarrierAccessFlags::kShaderRead;
        m_resourcePool->CommitBufferMemory(chunk.m_buffer.Get(), commitParams);

        chunk.m_hostPointer = static_cast<std::byte*>(chunk.m_buffer->Map());
        chunk.m_deviceAddress = chunk.m_buffer->GetDeviceAddress();
        return chunk;
    }
} // namespace FE::Graphics::Common
