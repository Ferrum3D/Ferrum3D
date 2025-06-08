#include <Graphics/Assets/TextureAssetManager.h>

namespace FE::Graphics
{
    TextureAssetManager::TextureAssetManager(Logger* logger, IJobSystem* jobSystem, IO::IAsyncStreamIO* asyncIO,
                                             Core::ResourcePool* resourcePool, Core::AsyncCopyQueue* asyncCopy)
        : m_logger(logger)
        , m_jobSystem(jobSystem)
        , m_asyncIO(asyncIO)
        , m_resourcePool(resourcePool)
        , m_asyncCopy(asyncCopy)
    {
    }


    TextureAsset* TextureAssetManager::Load(const Env::Name assetName)
    {
        FE_PROFILER_ZONE();

        std::lock_guard lock{ m_lock };

        Request* request = m_requestPool.New();
        request->m_asset = Rc<TextureAsset>::New(m_assetPool.GetAllocator());
        request->m_stage = LoadingStage::kHeader;
        request->m_asset->m_name = assetName;
        request->m_asset->m_completionWaitGroup = WaitGroup::Create();

        IO::AsyncBlockReadRequest readRequest;
        readRequest.m_path = IO::GetAbsolutePath(festd::string_view(assetName));
        readRequest.m_callback = this;
        readRequest.m_userData0 = reinterpret_cast<uintptr_t>(request);
        m_asyncIO->ReadAsync(readRequest);

        return request->m_asset.Get();
    }


    void TextureAssetManager::MipFinalizerJob::Execute()
    {
        FE_PROFILER_ZONE_NAMED("FinalizeMipChain");

        m_uploadWaitGroup->Wait();

        std::lock_guard lock{ m_request->m_loadedMipChainsMaskLock };
        FE_AssertDebug(!m_request->m_loadedMipChainsMask.test(m_mipChainIndex));
        m_request->m_loadedMipChainsMask.set(m_mipChainIndex);
        const uint32_t loadedMipCount = Bit::PopCount(m_request->m_loadedMipChainsMask.view());

        if (loadedMipCount == m_request->m_mipChains.size())
        {
            m_request->m_asset->m_status.store(AssetLoadingStatus::kCompletelyLoaded, std::memory_order_release);
            m_request->m_asset->m_completionWaitGroup->Signal();
            m_manager->m_requestPool.Delete(m_request);
        }
        else
        {
            m_request->m_asset->m_status.store(AssetLoadingStatus::kHasLoadedMips, std::memory_order_release);
        }

        m_manager->m_mipFinalizerJobPool.Delete(this);

        const uint32_t allocSize = m_request->m_mipChains[m_mipChainIndex].m_loadedBlockCount * Compression::kBlockSize;
        if (m_bufferAllocator)
            m_bufferAllocator->deallocate(const_cast<std::byte*>(m_data), allocSize);
    }


    void TextureAssetManager::AsyncIOCallback(const IO::AsyncBlockReadResult& result)
    {
        FE_PROFILER_ZONE();

        IO::AsyncBlockReadRequest* readRequest = result.m_request;
        auto* request = reinterpret_cast<Request*>(readRequest->m_userData0);

        if (result.m_controller->GetStatus() == IO::AsyncOperationStatus::kFailed)
        {
            request->m_asset->m_status.store(AssetLoadingStatus::kFailed, std::memory_order_release);
            request->m_asset->m_completionWaitGroup->Signal();
            m_requestPool.Delete(request);
            return;
        }

        const festd::span data{ readRequest->m_readBuffer, static_cast<uint32_t>(result.m_bytesRead) };

        switch (request->m_stage)
        {
        default:
            FE_DebugBreak();
            [[fallthrough]];

        case LoadingStage::kHeader:
            if (!OnHeaderLoaded(data, request, readRequest->m_stream.Get()))
            {
                request->m_asset->m_status.store(AssetLoadingStatus::kFailed, std::memory_order_release);
                request->m_asset->m_completionWaitGroup->Signal();
                m_requestPool.Delete(request);
            }
            break;

        case LoadingStage::kMips:
            if (!OnMipChainLoaded(readRequest->m_readBuffer,
                                  request,
                                  static_cast<uint32_t>(readRequest->m_userData1),
                                  readRequest->m_allocator))
            {
                request->m_asset->m_status.store(AssetLoadingStatus::kFailed, std::memory_order_release);
                request->m_asset->m_completionWaitGroup->Signal();
                m_requestPool.Delete(request);
            }
            break;
        }
    }


    bool TextureAssetManager::OnHeaderLoaded(const festd::span<const std::byte> data, Request* request, IO::IStream* stream)
    {
        FE_PROFILER_ZONE();

        Memory::BlockReader reader{ data };

        if (reader.AvailableSpace() < sizeof(Data::TextureHeader))
            return false;

        const auto header = reader.Read<Data::TextureHeader>();

        if (header.m_magic != Data::kTextureMagic)
            return false;

        if (header.m_desc.m_mipSliceCount > Core::Limits::Image::kMaxMipCount)
            return false;

        FE_Assert(header.m_desc.m_arraySize == 1, "Not implemented");

        for (uint32_t i = 0; i < header.m_desc.m_mipSliceCount; ++i)
        {
            if (reader.AvailableSpace() < sizeof(Data::MipChainInfo))
                return false;

            const auto mipInfo = reader.Read<Data::MipChainInfo>();
            auto& mipRequest = request->m_mipChains.push_back();
            mipRequest.m_info = mipInfo;
        }

        request->m_loadedMipChainsMask.resize(request->m_mipChains.size(), false);

        request->m_header = header;
        request->m_stage = LoadingStage::kMips;

        request->m_asset->m_resource = m_resourcePool->CreateTexture(request->m_asset->m_name, request->m_header.m_desc);

        for (uint32_t i = 0; i < request->m_mipChains.size(); ++i)
        {
            Request::MipChainRequest& mipRequest = request->m_mipChains[i];
            const Data::MipChainInfo mipChain = mipRequest.m_info;
            const Core::FormatInfo formatInfo{ request->m_header.m_desc.m_imageFormat };

            uint32_t mipChainByteSize = 0;
            for (uint32_t j = 0; j < mipChain.m_mipSliceCount; ++j)
            {
                const uint32_t mipSlice = mipChain.m_mostDetailedMipSlice + j;
                mipChainByteSize += formatInfo.CalculateMipByteSize(request->m_header.m_desc.GetSize(), mipSlice);
            }

            if (reader.AvailableSpace() > 0)
            {
                // The least detailed mip chain might be in the same block as the header.
                if (!OnMipChainLoaded(reader.m_ptr, request, i, nullptr))
                    return false;

                reader.m_ptr = reader.m_end;
            }
            else
            {
                IO::AsyncBlockReadRequest mipBlockReadRequest;
                mipBlockReadRequest.m_callback = this;
                mipBlockReadRequest.m_userData0 = reinterpret_cast<uintptr_t>(request);
                mipBlockReadRequest.m_userData1 = i;
                mipBlockReadRequest.m_stream = stream;
                mipBlockReadRequest.m_blockCount = mipChain.m_blockCount;
                m_asyncIO->ReadAsync(mipBlockReadRequest, IO::Priority::kNormal + static_cast<int32_t>(i));
            }
        }
        return true;
    }


    bool TextureAssetManager::OnMipChainLoaded(const std::byte* data, Request* request, const uint32_t mipChainIndex,
                                               std::pmr::memory_resource* bufferAllocator)
    {
        FE_PROFILER_ZONE();

        Request::MipChainRequest& mipRequest = request->m_mipChains[mipChainIndex];
        const Data::MipChainInfo mipInfo = mipRequest.m_info;

        Core::AsyncCopyCommandListBuilder copyCommandListBuilder{ &m_asyncCopyCommandPagePool, kAsyncCopyCommandSegmentSize };

        const uint32_t loadedBlockCount = mipRequest.m_loadedBlockCount.fetch_add(1, std::memory_order_acq_rel);
        if (loadedBlockCount + 1 < mipInfo.m_blockCount)
            return true;

        Core::ImageSubresource subresource;
        subresource.m_mostDetailedMipSlice = mipInfo.m_mostDetailedMipSlice;
        subresource.m_mipSliceCount = mipInfo.m_mipSliceCount;
        subresource.m_firstArraySlice = mipInfo.m_arraySlice;
        subresource.m_arraySize = 1;
        copyCommandListBuilder.UploadTexture(request->m_asset->m_resource.Get(), data, 0, subresource);

        const Rc uploadWaitGroup = WaitGroup::Create();
        auto* copyCommandList = Memory::New<Core::AsyncCopyCommandList>(&m_asyncCopyCommandListPool,
                                                                        copyCommandListBuilder.Build(uploadWaitGroup.Get()));
        m_asyncCopy->ExecuteCommandList(copyCommandList);

        auto* mipJob = m_mipFinalizerJobPool.New();
        mipJob->m_manager = this;
        mipJob->m_uploadWaitGroup = uploadWaitGroup;
        mipJob->m_request = request;
        mipJob->m_mipChainIndex = mipChainIndex;
        mipJob->m_data = data;
        mipJob->m_bufferAllocator = bufferAllocator;
        mipJob->ScheduleBackground(m_jobSystem);

        return true;
    }
} // namespace FE::Graphics
