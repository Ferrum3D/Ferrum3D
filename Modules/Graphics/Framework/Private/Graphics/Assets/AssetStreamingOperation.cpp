#include <Core/Utils/Crc32.h>
#include <Graphics/Assets/AssetStreamingOperation.h>
#include <mutex>

namespace FE::Graphics
{
    namespace
    {
        uint32_t GetPayloadSize(const IO::ArtifactPayloadRecord& payload)
        {
            uint64_t size = 0;
            for (const IO::ArtifactChunkRecord& chunk : payload.m_chunks)
                size += chunk.m_uncompressedSize;

            FE_Assert(size <= Constants::kMaxU32);
            return static_cast<uint32_t>(size);
        }


        bool ValidatePayload(const IO::ArtifactPayloadRecord& payload, const festd::span<const std::byte> data)
        {
            size_t offset = 0;
            for (const IO::ArtifactChunkRecord& chunk : payload.m_chunks)
            {
                if (offset + chunk.m_uncompressedSize > data.size()
                    || Crc32::Compute(data.data() + offset, chunk.m_uncompressedSize) != chunk.m_checksum.m_current)
                {
                    return false;
                }
                offset += chunk.m_uncompressedSize;
            }
            return offset == data.size();
        }
    } // namespace


    AssetStreamingOperation::AssetStreamingOperation(const AssetStreamingOperationType type, const uint32_t targetIndex,
                                                     Core::AsyncCopyQueue* asyncCopyQueue)
        : m_type(type)
        , m_targetIndex(targetIndex)
        , m_asyncCopyQueue(asyncCopyQueue)
        , m_prepareDone(WaitGroup::Create())
        , m_uploadDone(WaitGroup::Create())
    {
        FE_Assert(asyncCopyQueue);
    }


    bool AssetStreamingOperation::ReadPayloads(const IO::ArtifactRecord& artifact, const uint32_t firstPayload,
                                               const uint32_t payloadCount)
    {
        if (payloadCount == 0)
            return !IsCancellationRequested();
        if (firstPayload >= artifact.m_payloads.size() || payloadCount > artifact.m_payloads.size() - firstPayload)
            return false;

        const Rc<WaitGroup> readsDone = WaitGroup::Create(payloadCount);
        {
            std::lock_guard lock{ m_readMutex };
            if (IsCancellationRequested())
                return false;

            m_reads.resize(payloadCount);
            for (uint32_t payloadOffset = 0; payloadOffset < payloadCount; ++payloadOffset)
            {
                const uint32_t payloadIndex = firstPayload + payloadOffset;
                const IO::ArtifactPayloadRecord& payload = artifact.m_payloads[payloadIndex];
                PayloadRead& read = m_reads[payloadOffset];
                read.m_payloadIndex = payloadIndex;
                read.m_data.reserve(GetPayloadSize(payload));

                IO::Async::Batch batch(payload.m_resolvedDataSource, readsDone.Get());
                for (const IO::ArtifactChunkRecord& chunk : payload.m_chunks)
                {
                    batch.ReadAppend(read.m_data,
                                     chunk.m_compressionMethod,
                                     chunk.m_compressedSize,
                                     chunk.m_uncompressedSize,
                                     chunk.m_offsetInPayload);
                }
                read.m_controller = IO::Async::Read(std::move(batch));
            }
        }

        readsDone->Wait();
        if (IsCancellationRequested())
            return false;

        for (const PayloadRead& read : m_reads)
        {
            if (read.m_controller->GetStatus() != IO::Async::Status::kSucceeded
                || !ValidatePayload(artifact.m_payloads[read.m_payloadIndex], read.m_data))
            {
                return false;
            }
        }
        return true;
    }


    void AssetStreamingOperation::CancelAndWait()
    {
        m_cancelRequested.store(true, std::memory_order_release);
        {
            std::lock_guard lock{ m_readMutex };
            for (PayloadRead& read : m_reads)
            {
                if (read.m_controller && !IO::Async::IsFinalStatus(read.m_controller->GetStatus()))
                    read.m_controller->Cancel();
            }
        }

        if (!m_prepareDone->IsSignaled())
            m_prepareDone->Wait();

        // Prepared commands are still owned here until Tick submits them to the copy queue.
        if (m_commandList)
        {
            m_commandList->m_buffer.Free();
            Memory::Delete(m_commandList->m_allocator, m_commandList);
            m_commandList = nullptr;
        }

        if (m_uploadSubmitted && !m_uploadDone->IsSignaled())
            m_asyncCopyQueue->Drain();
    }


    bool AssetStreamingOperation::IsCancellationRequested() const
    {
        return m_cancelRequested.load(std::memory_order_acquire);
    }
} // namespace FE::Graphics
