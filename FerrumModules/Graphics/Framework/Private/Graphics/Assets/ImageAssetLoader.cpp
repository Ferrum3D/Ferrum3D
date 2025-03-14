#include <FeCore/DI/BaseDI.h>
#include <FeCore/Logging/Trace.h>
#include <Graphics/Assets/ImageAssetLoader.h>
#include <Graphics/Assets/ImageAssetStorage.h>
#include <Graphics/Assets/ImageLoaderImpl.h>
#include <Graphics/Core/Buffer.h>
#include <Graphics/Core/CommandList.h>
#include <Graphics/Core/CommandQueue.h>
#include <Graphics/Core/Fence.h>

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

            PixelBuffer = { result.m_request->m_readBuffer, result.m_request->m_readBufferSize };
            Schedule(pLoader->m_jobSystem);
        }

        inline void Execute() override
        {
            ZoneScoped;

            DI::IServiceProvider* pServiceProvider = Env::GetServiceProvider();
            auto* resourcePool = pServiceProvider->ResolveRequired<Core::ResourcePool>();

            const Core::ImageDesc imageDesc = pStorage->GetImage()->GetDesc();
            const Core::BufferDesc stagingBufferDesc{ MipSize, Core::BindFlags::kNone, Core::ResourceUsage::kHostWriteThrough };
            const auto bufferName = Fmt::FixedFormat("Staging Buffer {}", pStorage->GetImage()->GetName());
            const Rc stagingBuffer = resourcePool->CreateBuffer(Env::Name{ bufferName }, stagingBufferDesc).value();
            stagingBuffer->UpdateData(PixelBuffer.data(), 0, PixelBuffer.size_bytes());

            // TODO: async copy...
            const Rc commandList = pServiceProvider->ResolveRequired<Core::CommandList>();
            commandList->Init({ Core::HardwareQueueKindFlags::kTransfer, Core::CommandListFlags::kOneTimeSubmit });
            commandList->Begin();

            Core::ImageSubresourceRange subresourceRange;
            subresourceRange.m_aspectFlags = Core::ImageAspectFlags::kColor;
            subresourceRange.m_minArraySlice = 0;
            subresourceRange.m_arraySliceCount = 1;
            subresourceRange.m_minMipSlice = MipIndex;
            subresourceRange.m_mipSliceCount = 1;

            Core::ImageBarrierDesc barrier;
            barrier.m_image = pStorage->GetImage();
            barrier.m_subresourceRange = subresourceRange;
            barrier.m_stateAfter = Core::ResourceState::kTransferWrite;
            commandList->ResourceTransitionBarriers(std::array{ barrier }, {});

            Core::BufferImageCopyRegion copyRegion;
            copyRegion.m_imageSubresource.m_mipSlice = MipIndex;
            copyRegion.m_imageSubresource.m_arraySlice = 0;
            copyRegion.m_imageSubresource.m_aspect = Core::ImageAspect::kColor;
            copyRegion.m_bufferOffset = 0;
            copyRegion.m_imageOffset = { 0, 0, 0 };
            copyRegion.m_imageSize = { imageDesc.m_width >> MipIndex,
                                       std::max(1u, imageDesc.m_height >> MipIndex),
                                       std::max(1u, imageDesc.m_depth >> MipIndex) };

            commandList->CopyBufferToImage(stagingBuffer.Get(), pStorage->GetImage(), copyRegion);

            barrier.m_stateBefore = Core::ResourceState::kTransferWrite;
            barrier.m_stateAfter = Core::ResourceState::kShaderResource;
            commandList->ResourceTransitionBarriers(std::array{ barrier }, {});

            commandList->End();

            const Rc fence = pServiceProvider->ResolveRequired<Core::Fence>();
            fence->Init();

            Core::Device* device = commandList->GetDevice();
            Rc<Core::CommandQueue> transferQueue = device->GetCommandQueue(Core::HardwareQueueKindFlags::kTransfer);
            transferQueue->Execute(std::array{ commandList.Get() });
            transferQueue->SignalFence({ fence, 1 });
            fence->Wait(1);

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

        FE_Assert(result.m_controller->GetStatus() == IO::AsyncOperationStatus::kSucceeded);

        ImageAssetStorage* storage = static_cast<ImageAssetStorage*>(result.m_request->m_userData);
        const char* signature = reinterpret_cast<const char*>(result.m_request->m_readBuffer);
        FE_Assert(Str::ByteCompare(signature, "DDS ", 4) == 0);

        const DDS::Header* header = reinterpret_cast<DDS::Header*>(result.m_request->m_readBuffer + 4);
        const DDS::BaseHeader* baseHeader = &header->header;
        const DDS::HeaderDXT10* dx10Header = &header->headerDX10;
        FE_Assert(result.m_bytesRead == sizeof(DDS::Header) + 4);
        FE_Verify(DDS::CheckHeader(header));

        storage->m_loadingState.store(ImageAssetStorage::LoadingState::kHeaderLoaded);

        Core::ImageDesc desc;
        desc.m_sampleCount = 1;
        desc.m_width = baseHeader->dwWidth;
        desc.m_height = baseHeader->dwHeight;
        desc.m_depth = std::max(baseHeader->dwDepth, 1u);
        desc.m_mipSliceCount = baseHeader->dwMipMapCount;
        desc.m_arraySize = dx10Header->arraySize;
        desc.m_imageFormat = DDS::ConvertFormat(dx10Header->dxgiFormat);
        desc.m_bindFlags = Core::ImageBindFlags::kShaderRead | Core::ImageBindFlags::kTransferWrite;
        desc.m_dimension = DDS::ConvertDimension(dx10Header->resourceDimension);

        auto* resourcePool = Env::GetServiceProvider()->ResolveRequired<Core::ResourcePool>();

        storage->m_image = resourcePool->CreateImage(Env::Name{ result.m_request->m_path }, desc).value();

        result.m_request->m_allocator->deallocate(result.m_request->m_readBuffer, result.m_request->m_readBufferSize);

        uint32_t currentMipOffset = sizeof(DDS::Header) + 4;
        festd::fixed_vector<uint32_t, 16> mipOffsets;
        festd::fixed_vector<uint32_t, 16> mipSizes;
        for (uint32_t mipIndex = 0; mipIndex < desc.m_mipSliceCount; ++mipIndex)
        {
            const Core::FormatInfo formatInfo{ desc.m_imageFormat };
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
            request.m_stream = result.m_request->m_stream;
            request.m_readBufferSize = mipSizes[mipIndex];
            request.m_offset = mipOffsets[mipIndex];
            request.m_priority = result.m_request->m_priority + mipIndex;
            request.m_callback = job;
            m_asyncStreamIO->ReadAsync(request);
        }
    }


    ImageAssetLoader::ImageAssetLoader(IO::IStreamFactory* pStreamFactory, IO::IAsyncStreamIO* pAsyncIO, IJobSystem* pJobSystem)
        : m_uploadJobPool("ImageAssetLoader/UploadJob", sizeof(MipUploadJob))
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

        const festd::string_view pathSlice = assetName;
        const IO::Path ddsPath = IO::Path{ pathSlice } + ".dds";
        if (m_streamFactory->FileExists(ddsPath))
        {
            IO::AsyncReadRequest request;
            request.m_path = ddsPath;
            request.m_readBufferSize = sizeof(DDS::Header) + 4;
            request.m_userData = fe_assert_cast<ImageAssetStorage*>(storage);
            request.m_callback = this;
            storage->AddStrongRef();
            m_asyncStreamIO->ReadAsync(request);
            return;
        }

        FE_DebugBreak();
    }
} // namespace FE::Graphics
