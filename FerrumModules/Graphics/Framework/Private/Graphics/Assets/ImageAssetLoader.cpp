#include "Graphics/RHI/ResourcePool.h"


#include <FeCore/DI/BaseDI.h>
#include <FeCore/Logging/Trace.h>
#include <Graphics/Assets/ImageAssetLoader.h>
#include <Graphics/Assets/ImageAssetStorage.h>
#include <Graphics/Assets/ImageLoaderImpl.h>
#include <Graphics/RHI/Buffer.h>
#include <Graphics/RHI/CommandList.h>
#include <Graphics/RHI/CommandQueue.h>
#include <Graphics/RHI/Fence.h>

namespace FE::Graphics
{
    struct ImageAssetLoader::MipUploadJob final
        : public Job
        , public IO::IAsyncReadCallback
    {
        ImageAssetLoader* pLoader = nullptr;
        ImageAssetStorage* pStorage = nullptr;
        festd::span<std::byte> PixelBuffer;
        uint8_t MipIndex = 0;
        uint32_t MipSize = 0;

        inline void AsyncIOCallback(const IO::AsyncReadResult& result) override
        {
            ZoneScoped;

            PixelBuffer = { result.pRequest->pReadBuffer, result.pRequest->ReadBufferSize };
            Schedule(pLoader->m_jobSystem);
        }

        inline void Execute() override
        {
            ZoneScoped;

            DI::IServiceProvider* pServiceProvider = Env::GetServiceProvider();
            auto* resourcePool = pServiceProvider->ResolveRequired<RHI::ResourcePool>();

            const RHI::ImageDesc imageDesc = pStorage->GetImage()->GetDesc();
            const RHI::BufferDesc stagingBufferDesc{ MipSize, RHI::BindFlags::kNone, RHI::ResourceUsage::kHostWriteThrough };
            const auto bufferName = Fmt::FixedFormat("Staging Buffer {}", pStorage->GetImage()->GetName());
            const Rc stagingBuffer = resourcePool->CreateBuffer(Env::Name{ bufferName }, stagingBufferDesc).value();
            stagingBuffer->UpdateData(PixelBuffer.data(), 0, PixelBuffer.size_bytes());

            // TODO: async copy...
            const Rc commandList = pServiceProvider->ResolveRequired<RHI::CommandList>();
            commandList->Init({ RHI::HardwareQueueKindFlags::kTransfer, RHI::CommandListFlags::kOneTimeSubmit });
            commandList->Begin();

            RHI::ImageSubresourceRange subresourceRange;
            subresourceRange.m_aspectFlags = RHI::ImageAspectFlags::kColor;
            subresourceRange.m_minArraySlice = 0;
            subresourceRange.m_arraySliceCount = 1;
            subresourceRange.m_minMipSlice = MipIndex;
            subresourceRange.m_mipSliceCount = 1;

            RHI::ImageBarrierDesc barrier;
            barrier.m_image = pStorage->GetImage();
            barrier.m_subresourceRange = subresourceRange;
            barrier.m_stateAfter = RHI::ResourceState::kTransferWrite;
            commandList->ResourceTransitionBarriers(std::array{ barrier }, {});

            RHI::BufferImageCopyRegion copyRegion;
            copyRegion.m_imageSubresource.m_mipSlice = MipIndex;
            copyRegion.m_imageSubresource.m_arraySlice = 0;
            copyRegion.m_imageSubresource.m_aspect = RHI::ImageAspect::kColor;
            copyRegion.m_bufferOffset = 0;
            copyRegion.m_imageOffset = { 0, 0, 0 };
            copyRegion.m_imageSize = { imageDesc.m_width >> MipIndex,
                                       std::max(1u, imageDesc.m_height >> MipIndex),
                                       std::max(1u, imageDesc.m_depth >> MipIndex) };

            commandList->CopyBufferToImage(stagingBuffer.Get(), pStorage->GetImage(), copyRegion);

            barrier.m_stateBefore = RHI::ResourceState::kTransferWrite;
            barrier.m_stateAfter = RHI::ResourceState::kShaderResource;
            commandList->ResourceTransitionBarriers(std::array{ barrier }, {});

            commandList->End();

            const Rc fence = pServiceProvider->ResolveRequired<RHI::Fence>();
            fence->Init();

            RHI::Device* device = commandList->GetDevice();
            Rc<RHI::CommandQueue> transferQueue = device->GetCommandQueue(RHI::HardwareQueueKindFlags::kTransfer);
            transferQueue->Execute(std::array{ commandList.Get() });
            transferQueue->SignalFence({ fence, 1 });
            fence->Wait(1);

            pStorage->m_imageView = pServiceProvider->ResolveRequired<RHI::ImageView>();
            pStorage->m_imageView->Init(RHI::ImageViewDesc::ForImage(pStorage->m_image.Get(), RHI::ImageAspectFlags::kColor));

            if (pStorage->m_loadedMipCount.fetch_add(1) == imageDesc.m_mipSliceCount - 1)
            {
                pStorage->m_loadingState = ImageAssetStorage::LoadingState::kCompleted;
            }
            else
            {
                ImageAssetStorage::LoadingState expectedState = ImageAssetStorage::LoadingState::kHeaderLoaded;
                pStorage->m_loadingState.compare_exchange_strong(expectedState, ImageAssetStorage::LoadingState::kHasMips);
            }

            Memory::DefaultFree(PixelBuffer.data());
            Memory::Delete(&pLoader->m_uploadJobPool, this, sizeof(MipUploadJob));
        }
    };


    void ImageAssetLoader::AsyncIOCallback(const IO::AsyncReadResult& result)
    {
        ZoneScoped;

        FE_Assert(result.pController->GetStatus() == IO::AsyncOperationStatus::kSucceeded);

        ImageAssetStorage* storage = static_cast<ImageAssetStorage*>(result.pRequest->pUserData);
        const char* signature = reinterpret_cast<const char*>(result.pRequest->pReadBuffer);
        FE_Assert(Str::ByteCompare(signature, "DDS ", 4) == 0);

        const DDS::Header* header = reinterpret_cast<DDS::Header*>(result.pRequest->pReadBuffer + 4);
        const DDS::BaseHeader* baseHeader = &header->header;
        const DDS::HeaderDXT10* dx10Header = &header->headerDX10;
        FE_Assert(result.BytesRead == sizeof(DDS::Header) + 4);
        FE_Verify(DDS::CheckHeader(header));

        storage->m_loadingState.store(ImageAssetStorage::LoadingState::kHeaderLoaded);

        RHI::ImageDesc desc;
        desc.m_sampleCount = 1;
        desc.m_width = baseHeader->dwWidth;
        desc.m_height = baseHeader->dwHeight;
        desc.m_depth = std::max(baseHeader->dwDepth, 1u);
        desc.m_mipSliceCount = baseHeader->dwMipMapCount;
        desc.m_arraySize = dx10Header->arraySize;
        desc.m_imageFormat = DDS::ConvertFormat(dx10Header->dxgiFormat);
        desc.m_bindFlags = RHI::ImageBindFlags::kShaderRead | RHI::ImageBindFlags::kTransferWrite;
        desc.m_dimension = DDS::ConvertDimension(dx10Header->resourceDimension);

        auto* resourcePool = Env::GetServiceProvider()->ResolveRequired<RHI::ResourcePool>();

        storage->m_image = resourcePool->CreateImage(Env::Name{ result.pRequest->Path }, desc).value();

        result.pRequest->pAllocator->deallocate(result.pRequest->pReadBuffer, result.pRequest->ReadBufferSize);

        uint32_t currentMipOffset = sizeof(DDS::Header) + 4;
        festd::fixed_vector<uint32_t, 16> mipOffsets;
        festd::fixed_vector<uint32_t, 16> mipSizes;
        for (uint32_t mipIndex = 0; mipIndex < desc.m_mipSliceCount; ++mipIndex)
        {
            const RHI::FormatInfo formatInfo{ desc.m_imageFormat };
            const uint32_t mipSize = formatInfo.CalculateMipByteSize(desc.GetSize(), mipIndex);
            mipOffsets.push_back(currentMipOffset);
            mipSizes.push_back(mipSize);
            currentMipOffset += mipSize;
        }

        for (int32_t mipIndex = desc.m_mipSliceCount - 1; mipIndex >= 0; --mipIndex)
        {
            MipUploadJob* job = Memory::New<MipUploadJob>(&m_uploadJobPool);
            job->pLoader = this;
            job->pStorage = storage;
            job->MipIndex = static_cast<uint8_t>(mipIndex);
            job->MipSize = mipSizes[mipIndex];

            IO::AsyncReadRequest request;
            request.pStream = result.pRequest->pStream;
            request.ReadBufferSize = mipSizes[mipIndex];
            request.Offset = mipOffsets[mipIndex];
            request.Priority = result.pRequest->Priority + mipIndex;
            request.pCallback = job;
            m_asyncStreamIO->ReadAsync(request);
        }
    }


    ImageAssetLoader::ImageAssetLoader(IO::IStreamFactory* pStreamFactory, IO::IAsyncStreamIO* pAsyncIO, IJobSystem* pJobSystem)
        : m_uploadJobPool("ImageAssetLoader/UploadJob", sizeof(MipUploadJob), 64 * 1024)
        , m_streamFactory(pStreamFactory)
        , m_asyncStreamIO(pAsyncIO)
        , m_jobSystem(pJobSystem)
    {
        m_sourceExtensions.push_back(".png");
        m_sourceExtensions.push_back(".jpg");

        m_spec.m_assetTypeName = Env::Name{ ImageAssetStorage::kAssetTypeName };
        m_spec.m_fileExtension = ".dds";
        m_spec.m_sourceExtensions = m_sourceExtensions;
    }


    const Assets::AssetLoaderSpec& ImageAssetLoader::GetSpec() const
    {
        return m_spec;
    }


    void ImageAssetLoader::CreateStorage(Assets::AssetStorage** ppStorage)
    {
        *ppStorage = Rc<ImageAssetStorage>::DefaultNew(this);
        (*ppStorage)->AddStrongRef();
    }


    void ImageAssetLoader::LoadAsset(Assets::AssetStorage* storage, Env::Name assetName)
    {
        ZoneScoped;

        const StringSlice pathSlice = assetName;
        const IO::FixedPath ddsPath = IO::FixedPath{ pathSlice } + ".dds";
        if (m_streamFactory->FileExists(ddsPath))
        {
            IO::AsyncReadRequest request;
            request.Path = ddsPath;
            request.ReadBufferSize = sizeof(DDS::Header) + 4;
            request.pUserData = fe_assert_cast<ImageAssetStorage*>(storage);
            request.pCallback = this;
            storage->AddStrongRef();
            m_asyncStreamIO->ReadAsync(request);
            return;
        }

        FE_DebugBreak();
    }
} // namespace FE::Graphics
