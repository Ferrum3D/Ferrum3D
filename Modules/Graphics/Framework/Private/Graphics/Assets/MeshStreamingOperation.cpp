#include <Core/Jobs/Jobs.h>
#include <Graphics/Assets/MeshStreamingOperation.h>

namespace FE::Graphics
{
    namespace
    {
        constexpr uint32_t kCommandPageSize = 4096;
    }


    MeshStreamingOperation::MeshStreamingOperation(const AssetStreamingOperationType type, const MeshAsset& asset,
                                                   const IO::ArtifactRecord& artifact, const uint32_t targetLod,
                                                   Core::Device* device, Core::ResourcePool* resourcePool,
                                                   Core::AsyncCopyQueue* asyncCopyQueue)
        : AssetStreamingOperation(type, targetLod, asyncCopyQueue)
        , m_asset(asset)
        , m_artifact(artifact)
        , m_device(device)
        , m_resourcePool(resourcePool)
        , m_commandAllocator("MeshStreamingOperationCommands", kCommandPageSize)
    {
        FE_Assert(device && resourcePool);
    }


    void MeshStreamingOperation::Start()
    {
        Jobs::DispatchBackground(
            [this]() {
                Prepare();
            },
            m_prepareDone.Get());
    }


    void MeshStreamingOperation::Prepare()
    {
        if (!ReadPayloads(m_artifact, 1, m_targetIndex + 1) || IsCancellationRequested())
            return;

        uint64_t totalSize = 0;
        for (const PayloadRead& read : m_reads)
            totalSize += read.m_data.size();
        if (totalSize == 0 || totalSize > Constants::kMaxU32)
            return;

        m_buffer = Core::Buffer::CreateByteAddress(m_device, "StreamedMesh", static_cast<uint32_t>(totalSize));
        constexpr Core::ResourceCommitParams commitParams{ .m_bindFlags = Core::BarrierAccessFlags::kVertexBuffer
                                                               | Core::BarrierAccessFlags::kIndexBuffer
                                                               | Core::BarrierAccessFlags::kShaderRead
                                                               | Core::BarrierAccessFlags::kCopyDest,
                                                           .m_memory = Core::ResourceMemory::kDeviceLocal,
                                                           .m_queue = Core::DeviceQueueType::kTransfer };
        m_resourcePool->CommitBufferMemory(m_buffer.Get(), commitParams);

        Core::AsyncCopyCommandListBuilder builder(&m_commandAllocator, kCommandPageSize);
        uint32_t destinationOffset = 0;
        for (const PayloadRead& read : m_reads)
        {
            builder.UploadBuffer(m_buffer.Get(), read.m_data.data(), 0, destinationOffset, read.m_data.size());
            destinationOffset += read.m_data.size();
        }

        if (IsCancellationRequested())
            return;

        m_commandList = builder.Build(&m_commandAllocator, m_uploadDone.Get());
        m_prepareSucceeded = true;
    }


    AssetStreamingOperationStatus MeshStreamingOperation::Tick()
    {
        if (!m_prepareDone->IsSignaled())
            return AssetStreamingOperationStatus::kPending;
        if (!m_prepareSucceeded)
            return AssetStreamingOperationStatus::kFailed;

        if (!m_uploadSubmitted)
        {
            m_asyncCopyQueue->ExecuteCommandList(m_commandList);
            m_commandList = nullptr;
            m_uploadSubmitted = true;
        }

        return m_uploadDone->IsSignaled() ? AssetStreamingOperationStatus::kSucceeded : AssetStreamingOperationStatus::kPending;
    }


    void MeshStreamingOperation::Cancel()
    {
        CancelAndWait();
    }


    Rc<Core::Buffer> MeshStreamingOperation::TakeBuffer()
    {
        FE_Assert(m_prepareSucceeded && m_uploadDone->IsSignaled());
        return std::move(m_buffer);
    }
} // namespace FE::Graphics
