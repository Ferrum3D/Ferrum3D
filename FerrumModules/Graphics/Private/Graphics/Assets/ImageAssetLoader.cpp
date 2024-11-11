#include <FeCore/DI/BaseDI.h>
#include <FeCore/Logging/Trace.h>
#include <Graphics/Assets/ImageAssetLoader.h>
#include <Graphics/Assets/ImageAssetStorage.h>
#include <Graphics/Assets/ImageLoaderImpl.h>
#include <HAL/Buffer.h>
#include <HAL/CommandList.h>
#include <HAL/CommandQueue.h>
#include <HAL/Fence.h>

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
            Schedule(pLoader->m_pJobSystem);
        }

        inline void Execute() override
        {
            ZoneScoped;

            DI::IServiceProvider* pServiceProvider = Env::GetServiceProvider();

            Rc stagingBuffer = pServiceProvider->ResolveRequired<HAL::Buffer>();

            const HAL::ImageDesc imageDesc = pStorage->GetImage()->GetDesc();
            const HAL::FormatInfo formatInfo{ imageDesc.ImageFormat };
            const HAL::BufferDesc stagingBufferDesc{ MipSize, HAL::BindFlags::None };
            const auto bufferName = Fmt::FixedFormat("Staging {}", pStorage->GetImage()->GetName());
            stagingBuffer->Init(bufferName, stagingBufferDesc);
            stagingBuffer->AllocateMemory(HAL::MemoryType::kHostVisible);
            stagingBuffer->UpdateData(PixelBuffer.data(), PixelBuffer.size_bytes());

            // TODO: async copy...
            Rc commandList = pServiceProvider->ResolveRequired<HAL::CommandList>();
            commandList->Init({ HAL::HardwareQueueKindFlags::kTransfer, HAL::CommandListFlags::OneTimeSubmit });
            commandList->Begin();

            HAL::ImageSubresourceRange subresourceRange;
            subresourceRange.AspectFlags = HAL::ImageAspectFlags::kColor;
            subresourceRange.MinArraySlice = 0;
            subresourceRange.ArraySliceCount = 1;
            subresourceRange.MinMipSlice = MipIndex;
            subresourceRange.MipSliceCount = 1;

            HAL::ImageBarrierDesc barrier;
            barrier.Image = pStorage->GetImage();
            barrier.SubresourceRange = subresourceRange;
            barrier.StateAfter = HAL::ResourceState::kTransferWrite;
            commandList->ResourceTransitionBarriers(std::array{ barrier }, {});

            HAL::BufferImageCopyRegion copyRegion;
            copyRegion.ImageSubresource.MipSlice = MipIndex;
            copyRegion.ImageSubresource.ArraySlice = 0;
            copyRegion.ImageSubresource.Aspect = HAL::ImageAspect::kColor;
            copyRegion.BufferOffset = 0;
            copyRegion.ImageOffset = { 0, 0, 0 };
            copyRegion.ImageSize = { imageDesc.Width >> MipIndex,
                                     std::max(1u, imageDesc.Height >> MipIndex),
                                     std::max(1u, imageDesc.Depth >> MipIndex) };

            commandList->CopyBufferToImage(stagingBuffer.Get(), pStorage->GetImage(), copyRegion);

            barrier.StateBefore = HAL::ResourceState::kTransferWrite;
            barrier.StateAfter = HAL::ResourceState::kShaderResource;
            commandList->ResourceTransitionBarriers(std::array{ barrier }, {});

            commandList->End();

            Rc fence = pServiceProvider->ResolveRequired<HAL::Fence>();
            fence->Init(HAL::FenceState::Reset);

            Rc<HAL::Device> device = commandList->GetDevice();
            Rc<HAL::CommandQueue> transferQueue = device->GetCommandQueue(HAL::HardwareQueueKindFlags::kTransfer);
            transferQueue->SubmitBuffers(std::array{ commandList.Get() }, fence.Get(), HAL::SubmitFlags::None);
            fence->WaitOnCPU();

            pStorage->m_imageView = pServiceProvider->ResolveRequired<HAL::ImageView>();
            pStorage->m_imageView->Init(HAL::ImageViewDesc::ForImage(pStorage->m_image.Get(), HAL::ImageAspectFlags::kColor));

            if (pStorage->m_loadedMipCount.fetch_add(1) == imageDesc.MipSliceCount - 1)
            {
                pStorage->m_loadingState = ImageAssetStorage::LoadingState::kCompleted;
            }
            else
            {
                ImageAssetStorage::LoadingState expectedState = ImageAssetStorage::LoadingState::kHeaderLoaded;
                pStorage->m_loadingState.compare_exchange_strong(expectedState, ImageAssetStorage::LoadingState::kHasMips);
            }

            Memory::DefaultFree(PixelBuffer.data());
            Memory::Delete(&pLoader->m_UploadJobPool, this, sizeof(MipUploadJob));
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

        HAL::ImageDesc desc;
        desc.SampleCount = 1;
        desc.Width = baseHeader->dwWidth;
        desc.Height = baseHeader->dwHeight;
        desc.Depth = std::max(baseHeader->dwDepth, 1u);
        desc.MipSliceCount = baseHeader->dwMipMapCount;
        desc.ArraySize = dx10Header->arraySize;
        desc.ImageFormat = DDS::ConvertFormat(dx10Header->dxgiFormat);
        desc.SetDimension(DDS::ConvertDimension(dx10Header->resourceDimension));

        storage->m_image = Env::GetServiceProvider()->ResolveRequired<HAL::Image>();
        FE_Verify(storage->m_image->Init(result.pRequest->Path, desc) == HAL::ResultCode::Success);

        result.pRequest->pAllocator->deallocate(result.pRequest->pReadBuffer, result.pRequest->ReadBufferSize);

        uint32_t currentMipOffset = sizeof(DDS::Header) + 4;
        festd::fixed_vector<uint32_t, 16> mipOffsets;
        festd::fixed_vector<uint32_t, 16> mipSizes;
        for (uint32_t mipIndex = 0; mipIndex < desc.MipSliceCount; ++mipIndex)
        {
            const HAL::FormatInfo formatInfo{ desc.ImageFormat };
            const uint32_t mipSize = formatInfo.CalculateMipByteSize(desc.GetSize(), mipIndex);
            mipOffsets.push_back(currentMipOffset);
            mipSizes.push_back(mipSize);
            currentMipOffset += mipSize;
        }

        for (int32_t mipIndex = desc.MipSliceCount - 1; mipIndex >= 0; --mipIndex)
        {
            MipUploadJob* job = Memory::New<MipUploadJob>(&m_UploadJobPool);
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
            m_pAsyncStreamIO->ReadAsync(request);
        }
    }


    ImageAssetLoader::ImageAssetLoader(IO::IStreamFactory* pStreamFactory, IO::IAsyncStreamIO* pAsyncIO, IJobSystem* pJobSystem)
        : m_UploadJobPool("ImageAssetLoader/UploadJob", sizeof(MipUploadJob), 64 * 1024)
        , m_pStreamFactory(pStreamFactory)
        , m_pAsyncStreamIO(pAsyncIO)
        , m_pJobSystem(pJobSystem)
    {
        m_SourceExtensions.push_back(".png");
        m_SourceExtensions.push_back(".jpg");

        m_Spec.AssetTypeName = Env::Name{ ImageAssetStorage::kAssetTypeName };
        m_Spec.FileExtension = ".dds";
        m_Spec.SourceExtensions = m_SourceExtensions;
    }


    const Assets::AssetLoaderSpec& ImageAssetLoader::GetSpec() const
    {
        return m_Spec;
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
        if (m_pStreamFactory->FileExists(ddsPath))
        {
            IO::AsyncReadRequest request;
            request.Path = ddsPath;
            request.ReadBufferSize = sizeof(DDS::Header) + 4;
            request.pUserData = fe_assert_cast<ImageAssetStorage*>(storage);
            request.pCallback = this;
            storage->AddStrongRef();
            m_pAsyncStreamIO->ReadAsync(request);
            return;
        }

        FE_DebugBreak();
    }
} // namespace FE::Graphics
