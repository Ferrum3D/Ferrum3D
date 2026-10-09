#include <Core/Jobs/Jobs.h>
#include <Graphics/Assets/TextureStreamingOperation.h>

namespace FE::Graphics
{
    namespace
    {
        constexpr uint32_t kCommandPageSize = 4096;
    }


    TextureStreamingOperation::TextureStreamingOperation(const AssetStreamingOperationType type, const TextureAsset& asset,
                                                         const IO::ArtifactRecord& artifact, const uint32_t targetMip,
                                                         Core::Device* device, Core::ResourcePool* resourcePool,
                                                         Core::AsyncCopyQueue* asyncCopyQueue)
        : AssetStreamingOperation(type, targetMip, asyncCopyQueue)
        , m_asset(asset)
        , m_artifact(artifact)
        , m_device(device)
        , m_resourcePool(resourcePool)
        , m_commandAllocator("TextureStreamingOperationCommands", kCommandPageSize)
    {
        FE_Assert(device && resourcePool);
    }


    void TextureStreamingOperation::Start()
    {
        Jobs::DispatchBackground(
            [this] {
                Prepare();
            },
            m_prepareDone.Get());
    }


    void TextureStreamingOperation::Prepare()
    {
        FE_Assert(m_targetIndex < m_asset.m_desc.m_mipSliceCount);

        const uint32_t tailMipCount = m_asset.m_mipTailOffsets.size();
        const uint32_t payloadCount = m_targetIndex + 1 > tailMipCount ? m_targetIndex + 1 - tailMipCount : 0;
        if (!ReadPayloads(m_artifact, 1, payloadCount) || IsCancellationRequested())
            return;

        Core::TextureDesc residentDesc = m_asset.m_desc;
        const uint32_t firstSourceMip = m_asset.m_desc.m_mipSliceCount - m_targetIndex - 1;
        residentDesc.m_width = Math::Max(residentDesc.m_width >> firstSourceMip, 1u);
        residentDesc.m_height = Math::Max(residentDesc.m_height >> firstSourceMip, 1u);
        if (residentDesc.m_dimension == Core::TextureDimension::k3D)
            residentDesc.m_depth = Math::Max(residentDesc.m_depth >> firstSourceMip, 1u);
        residentDesc.m_mipSliceCount = m_targetIndex + 1;

        m_texture = Core::Texture::Create(m_device, "StreamedTexture", residentDesc);
        constexpr Core::ResourceCommitParams commitParams{ .m_bindFlags = Core::BarrierAccessFlags::kShaderRead
                                                               | Core::BarrierAccessFlags::kCopyDest,
                                                           .m_memory = Core::ResourceMemory::kDeviceLocal,
                                                           .m_queue = Core::DeviceQueueType::kTransfer };
        m_resourcePool->CommitTextureMemory(m_texture.Get(), commitParams);

        Core::AsyncCopyCommandListBuilder builder(&m_commandAllocator, kCommandPageSize);
        for (uint32_t assetMip = 0; assetMip < tailMipCount && assetMip <= m_targetIndex; ++assetMip)
        {
            const uint32_t gpuMip = m_targetIndex - assetMip;
            const auto subresource =
                Core::TextureSubresource::Create(residentDesc, gpuMip, 0).SliceArray(0, residentDesc.m_arraySize);
            builder.UploadTexture(m_texture.Get(), m_asset.m_mipTailData.data(), m_asset.m_mipTailOffsets[assetMip], subresource);
        }

        for (uint32_t readIndex = 0; readIndex < m_reads.size(); ++readIndex)
        {
            const uint32_t assetMip = tailMipCount + readIndex;
            const uint32_t gpuMip = m_targetIndex - assetMip;
            const auto subresource =
                Core::TextureSubresource::Create(residentDesc, gpuMip, 0).SliceArray(0, residentDesc.m_arraySize);
            builder.UploadTexture(m_texture.Get(), m_reads[readIndex].m_data.data(), 0, subresource);
        }

        // Always transfer builder allocations to the operation so cancellation can release them.
        m_commandList = builder.Build(&m_commandAllocator, m_uploadDone.Get());
        m_prepareSucceeded = true;
    }


    AssetStreamingOperationStatus TextureStreamingOperation::Tick()
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


    void TextureStreamingOperation::Cancel()
    {
        CancelAndWait();
    }


    Rc<Core::Texture> TextureStreamingOperation::TakeTexture()
    {
        FE_Assert(m_prepareSucceeded && m_uploadDone->IsSignaled());
        return std::move(m_texture);
    }
} // namespace FE::Graphics
