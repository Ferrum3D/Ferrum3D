#include <Graphics/Assets/TextureAssetManager.h>

namespace FE::Graphics
{
    TextureAssetManager::TextureAssetManager(Logger* logger, IO::IAsyncStreamIO* asyncIO, Core::ResourcePool* resourcePool)
        : m_logger(logger)
        , m_asyncIO(asyncIO)
        , m_resourcePool(resourcePool)
    {
    }


    TextureAsset* TextureAssetManager::Load(const Env::Name assetName)
    {
        std::lock_guard lock{ m_lock };

        Request* request = m_requestPool.New();
        request->m_asset = Rc<TextureAsset>::New(m_assetPool.GetAllocator());
        request->m_stage = LoadingStage::kHeader;

        request->m_asset->m_name = assetName;
        request->m_asset->m_status.store(AssetLoadingStatus::kLoading, std::memory_order_relaxed);

        IO::AsyncBlockReadRequest readRequest;
        readRequest.m_path = IO::GetAbsolutePath(festd::string_view(assetName));
        readRequest.m_callback = this;
        readRequest.m_userData0 = reinterpret_cast<uintptr_t>(request);
        m_asyncIO->ReadAsync(readRequest);

        return request->m_asset.Get();
    }


    void TextureAssetManager::AsyncIOCallback(const IO::AsyncBlockReadResult& result)
    {
        IO::AsyncBlockReadRequest* readRequest = result.m_request;
        auto* request = reinterpret_cast<Request*>(readRequest->m_userData0);

        if (result.m_controller->GetStatus() == IO::AsyncOperationStatus::kFailed)
        {
            request->m_asset->m_status.store(AssetLoadingStatus::kFailed, std::memory_order_release);
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
            if (!OnHeaderLoaded(data, *request, readRequest->m_stream.Get()))
            {
                request->m_asset->m_status.store(AssetLoadingStatus::kFailed, std::memory_order_release);
                m_requestPool.Delete(request);
            }
            break;

        case LoadingStage::kMips:
            if (!OnMipChainLoaded(data, *request, static_cast<uint32_t>(readRequest->m_userData1)))
            {
                request->m_asset->m_status.store(AssetLoadingStatus::kFailed, std::memory_order_release);
                m_requestPool.Delete(request);
            }
            break;
        }
    }


    bool TextureAssetManager::OnHeaderLoaded(const festd::span<const std::byte> data, Request& request, IO::IStream* stream)
    {
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
            request.m_mipChains.push_back(mipInfo);
        }

        for (uint32_t i = 0; i < request.m_mipChains.size(); ++i)
        {
            const Data::MipChainInfo mipChain = request.m_mipChains[i];

            if (reader.AvailableSpace() > 0)
            {
                // The least detailed mip chain might be in the same block as the header.
                if (!OnMipChainLoaded({ reader.m_ptr, reader.m_end }, request, i))
                    return false;

                reader.m_ptr = reader.m_end;
            }
            else
            {
                IO::AsyncBlockReadRequest mipBlockReadRequest;
                mipBlockReadRequest.m_callback = this;
                mipBlockReadRequest.m_userData0 = reinterpret_cast<uintptr_t>(&request);
                mipBlockReadRequest.m_userData1 = i;
                mipBlockReadRequest.m_stream = stream;
                mipBlockReadRequest.m_blockCount = mipChain.m_blockCount;
                m_asyncIO->ReadAsync(mipBlockReadRequest, IO::Priority::kNormal + static_cast<int32_t>(i));
            }
        }

        request.m_header = header;
        request.m_stage = LoadingStage::kMips;
        return true;
    }


    bool TextureAssetManager::OnMipChainLoaded(festd::span<const std::byte> data, Request& request, uint32_t mipChainIndex) {}
} // namespace FE::Graphics
