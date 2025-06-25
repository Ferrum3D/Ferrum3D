#pragma once
#include <FeCore/IO/IAsyncStreamIO.h>
#include <FeCore/Jobs/Job.h>
#include <FeCore/Memory/PoolAllocator.h>
#include <Graphics/Assets/IModelAssetManager.h>
#include <Graphics/Assets/ModelAssetFormat.h>
#include <Graphics/Core/AsyncCopyQueue.h>
#include <Graphics/Core/ResourcePool.h>

namespace FE::Graphics
{
    struct ModelAssetManager final
        : public IModelAssetManager
        , public IO::IAsyncReadCallback
    {
        FE_RTTI_Class(ModelAssetManager, "6CA86F05-71EA-45D8-9D22-D57E469F4752");

        ModelAssetManager(Logger* logger, IJobSystem* jobSystem, IO::IAsyncStreamIO* asyncIO, Core::ResourcePool* resourcePool,
                          Core::AsyncCopyQueue* asyncCopy);

        ModelAsset* Load(Env::Name assetName) override;

    private:
        enum class LoadingStage : uint32_t
        {
            kHeaders,
            kLods,
        };

        struct Request final
        {
            Data::ModelHeader m_header;
            Rc<ModelAsset> m_asset;
            std::atomic<uint32_t>* m_lodLoadedBlockCount = nullptr;
            LoadingStage m_stage = LoadingStage::kHeaders;
            uint32_t m_loadedLods = 0;
        };

        void AsyncIOCallback(const IO::AsyncBlockReadResult& result) override;

        bool OnHeadersLoaded(festd::span<const std::byte> data, Request* request, IO::IStream* stream);
        bool OnLodDataLoaded(festd::span<std::byte> data, Request* request, uint32_t lodIndex, uint32_t expectedDataSize,
                             std::pmr::memory_resource* bufferAllocator);

        Logger* m_logger;
        IJobSystem* m_jobSystem;
        IO::IAsyncStreamIO* m_asyncIO;
        Core::ResourcePool* m_resourcePool;
        Core::AsyncCopyQueue* m_asyncCopy;

        Threading::SpinLock m_lock;
        Memory::Pool<ModelAsset> m_assetPool{ "ModelAssetPool" };
        Memory::Pool<Request> m_requestPool{ "ModelRequestPool" };

        static constexpr uint32_t kAsyncCopyCommandSegmentSize = 1024;
        Memory::SpinLockedPoolAllocator m_asyncCopyCommandPagePool{ "AsyncCopyCommandPagePool", kAsyncCopyCommandSegmentSize };
        Memory::SpinLockedPoolAllocator m_asyncCopyCommandListPool{ "AsyncCopyCommandListPool",
                                                                    sizeof(Core::AsyncCopyCommandList) };
    };
} // namespace FE::Graphics
