#include <Graphics/Assets/ModelAssetManager.h>
#include <Graphics/Core/AsyncCopyQueue.h>

namespace FE::Graphics
{
    namespace
    {
        bool LoadingFailed()
        {
            if (Build::IsDebug())
                FE_DebugBreak();

            return false;
        }
    } // namespace


    ModelAssetManager::ModelAssetManager(Logger* logger, IJobSystem* jobSystem, IO::IAsyncStreamIO* asyncIO,
                                         Core::ResourcePool* resourcePool, Core::AsyncCopyQueue* asyncCopy)
        : m_logger(logger)
        , m_jobSystem(jobSystem)
        , m_asyncIO(asyncIO)
        , m_resourcePool(resourcePool)
        , m_asyncCopy(asyncCopy)
    {
    }


    ModelAsset* ModelAssetManager::Load(const Env::Name assetName)
    {
        FE_PROFILER_ZONE();

        std::lock_guard lock{ m_lock };

        Request* request = m_requestPool.New();
        request->m_asset = Rc<ModelAsset>::New(m_assetPool.GetAllocator());
        request->m_stage = LoadingStage::kHeaders;
        request->m_asset->m_name = assetName;
        request->m_asset->m_completionWaitGroup = WaitGroup::Create();

        IO::AsyncBlockReadRequest readRequest;
        readRequest.m_path = IO::GetAbsolutePath(festd::string_view(assetName));
        readRequest.m_callback = this;
        readRequest.m_userData0 = reinterpret_cast<uintptr_t>(request);
        m_asyncIO->ReadAsync(readRequest);

        return request->m_asset.Get();
    }


    void ModelAssetManager::AsyncIOCallback(const IO::AsyncBlockReadResult& result)
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

        bool success;
        switch (request->m_stage)
        {
        default:
            FE_DebugBreak();
            [[fallthrough]];

        case LoadingStage::kHeaders:
            success = OnHeadersLoaded(data, request, readRequest->m_stream.Get());
            result.FreeData();
            break;

        case LoadingStage::kLods:
            success = OnLodDataLoaded(data,
                                      request,
                                      readRequest->m_userData1 >> 32,
                                      readRequest->m_userData1 & Constants::kMaxU32,
                                      readRequest->m_allocator);
            break;
        }

        if (!success)
        {
            request->m_asset->m_status.store(AssetLoadingStatus::kFailed, std::memory_order_release);
            request->m_asset->m_completionWaitGroup->Signal();
            m_requestPool.Delete(request);
        }
    }


    bool ModelAssetManager::OnHeadersLoaded(const festd::span<const std::byte> data, Request* request, IO::IStream* stream)
    {
        FE_PROFILER_ZONE();

        Memory::BlockReader reader{ data };

        if (reader.AvailableSpace() < sizeof(Data::ModelHeader))
            return LoadingFailed();

        const auto header = reader.Read<Data::ModelHeader>();

        if (header.m_magic != Data::kModelMagic)
            return LoadingFailed();

        request->m_header = header;
        request->m_asset->m_meshes.reserve(header.m_meshCount);
        request->m_asset->m_lods.reserve(header.m_lodCount * header.m_meshCount);
        request->m_lodLoadedBlockCount = Memory::DefaultAllocateArray<std::atomic<uint32_t>>(header.m_lodCount);
        memset(request->m_lodLoadedBlockCount, 0, sizeof(uint32_t) * header.m_lodCount);

        for (uint32_t meshIndex = 0; meshIndex < header.m_meshCount; ++meshIndex)
        {
            if (reader.AvailableSpace() < sizeof(Core::MeshInfo))
                return LoadingFailed();

            const Core::MeshInfo meshHeader = reader.Read<Core::MeshInfo>();
            request->m_asset->m_meshes.push_back(meshHeader);

            for (uint32_t lodIndex = 0; lodIndex < header.m_lodCount; ++lodIndex)
            {
                if (reader.AvailableSpace() < sizeof(Core::MeshLodInfo))
                    return LoadingFailed();

                request->m_asset->m_lods.push_back(reader.Read<Core::MeshLodInfo>());
            }
        }

        request->m_asset->m_lodCount = header.m_lodCount;
        request->m_asset->m_meshCount = header.m_meshCount;
        request->m_asset->m_geometryBuffers.resize(header.m_lodCount);

        request->m_asset->m_lodErrors.resize(header.m_lodCount - 1);
        if (header.m_lodCount > 1)
        {
            if (!reader.ReadBytes(request->m_asset->m_lodErrors.data(), festd::size_bytes(request->m_asset->m_lodErrors)))
                return LoadingFailed();
        }

        request->m_stage = LoadingStage::kLods;

        for (uint32_t lodIndex = 0; lodIndex < header.m_lodCount; ++lodIndex)
        {
            uint32_t dataSize = 0;
            for (uint32_t meshIndex = 0; meshIndex < header.m_meshCount; ++meshIndex)
            {
                const Core::MeshInfo& meshHeader = request->m_asset->m_meshes[meshIndex];
                const Core::MeshLodInfo& lodHeader = request->m_asset->m_lods[meshIndex * header.m_lodCount + lodIndex];

                const uint32_t vertexSize = meshHeader.m_layout.CalculateTotalStride();

                dataSize += vertexSize * lodHeader.m_vertexCount;
                dataSize += sizeof(uint32_t) * lodHeader.m_indexCount;
                dataSize += sizeof(Core::MeshletHeader) * lodHeader.m_meshletCount;
                dataSize += sizeof(Core::PackedTriangle) * lodHeader.m_primitiveCount;
            }

            IO::AsyncBlockReadRequest lodRequest;
            lodRequest.m_callback = this;
            lodRequest.m_userData0 = reinterpret_cast<uintptr_t>(request);
            lodRequest.m_userData1 = (static_cast<uintptr_t>(lodIndex) << 32) | dataSize;
            lodRequest.m_stream = stream;
            lodRequest.m_blockCount = Math::CeilDivide(dataSize, Compression::kBlockSize);
            m_asyncIO->ReadAsync(lodRequest, IO::Priority::kNormal);
        }

        return true;
    }


    bool ModelAssetManager::OnLodDataLoaded(const festd::span<std::byte> data, Request* request, const uint32_t lodIndex,
                                            const uint32_t expectedDataSize, std::pmr::memory_resource* bufferAllocator)
    {
        FE_PROFILER_ZONE();

        std::atomic<uint32_t>& blockCounter = request->m_lodLoadedBlockCount[lodIndex];
        const uint32_t loadedBlockCount = blockCounter.fetch_add(1, std::memory_order_acq_rel);
        const uint32_t expectedBlockCount = Math::CeilDivide(expectedDataSize, Compression::kBlockSize);
        if (loadedBlockCount + 1 < expectedBlockCount)
            return true;

        const Rc geometryBuffer = m_resourcePool->CreateBuffer(
            Fmt::FormatName("GeometryBuffer '{}' LOD{}", request->m_asset->m_name, request->m_header.m_lodCount - lodIndex - 1),
            Core::BufferDesc(expectedDataSize, Core::BindFlags::kShaderResource, Core::ResourceUsage::kDeviceOnly));

        request->m_asset->m_geometryBuffers[lodIndex] = geometryBuffer;

        Core::AsyncCopyCommandListBuilder copyCommandListBuilder{ &m_asyncCopyCommandPagePool, kAsyncCopyCommandSegmentSize };
        copyCommandListBuilder.UploadBuffer(geometryBuffer.Get(), data.data());
        copyCommandListBuilder.Invoke([this, bufferAllocator, data, request] {
            bufferAllocator->deallocate(data.data(), data.size_bytes());

            ++request->m_loadedLods;
            if (request->m_loadedLods == request->m_asset->m_lodCount)
            {
                Memory::DefaultFree(request->m_lodLoadedBlockCount);
                request->m_asset->m_status.store(AssetLoadingStatus::kCompletelyLoaded, std::memory_order_release);
                request->m_asset->m_completionWaitGroup->Signal();
                m_requestPool.Delete(request);
            }
            else if (request->m_loadedLods == 1)
            {
                request->m_asset->m_status.store(AssetLoadingStatus::kHasLoadedLods, std::memory_order_release);
            }
            else
            {
                FE_AssertDebug(request->m_asset->m_status.load(std::memory_order_relaxed) == AssetLoadingStatus::kHasLoadedLods);
            }
        });

        Core::AsyncCopyCommandList* copyCommandList = copyCommandListBuilder.Build(&m_asyncCopyCommandListPool);
        m_asyncCopy->ExecuteCommandList(copyCommandList);

        return true;
    }
} // namespace FE::Graphics
