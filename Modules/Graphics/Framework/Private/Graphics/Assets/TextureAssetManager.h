#pragma once
#include <FeCore/IO/IAsyncStreamIO.h>
#include <FeCore/Jobs/Job.h>
#include <FeCore/Memory/PoolAllocator.h>
#include <Graphics/Assets/ITextureAssetManager.h>
#include <Graphics/Assets/TextureAssetFormat.h>
#include <Graphics/Core/AsyncCopyQueue.h>
#include <Graphics/Core/Base/Limits.h>
#include <Graphics/Core/ResourcePool.h>
#include <festd/bit_vector.h>
#include <festd/vector.h>

namespace FE::Graphics
{
    struct TextureAssetManager final
        : public ITextureAssetManager
        , public IO::IAsyncReadCallback
    {
        FE_RTTI_Class(TextureAssetManager, "9BC819E7-F2CD-47D3-A84C-421CEF60FC98");

        TextureAssetManager(Logger* logger, IJobSystem* jobSystem, IO::IAsyncStreamIO* asyncIO, Core::ResourcePool* resourcePool,
                            Core::AsyncCopyQueue* asyncCopy);

        TextureAsset* Load(Env::Name assetName) override;

    private:
        enum class LoadingStage : uint32_t
        {
            kHeader,
            kMips,
        };

        struct Request final
        {
            struct MipChainRequest final
            {
                Data::MipChainInfo m_info;
                std::atomic<uint32_t> m_loadedBlockCount = 0;
            };

            Data::TextureHeader m_header;
            Rc<TextureAsset> m_asset;
            festd::fixed_vector<MipChainRequest, Core::Limits::Image::kMaxMipCount> m_mipChains;
            festd::fixed_bit_vector<Core::Limits::Image::kMaxMipCount> m_loadedMipChainsMask;
            Threading::SpinLock m_loadedMipChainsMaskLock;
            LoadingStage m_stage;
        };

        struct MipFinalizerJob final : public Job
        {
            void Execute() override;

            TextureAssetManager* m_manager = nullptr;
            Rc<WaitGroup> m_uploadWaitGroup;
            Request* m_request = nullptr;
            uint32_t m_mipChainIndex = 0;
            const std::byte* m_data = nullptr;
            std::pmr::memory_resource* m_bufferAllocator = nullptr;
            Core::AsyncCopyCommandList* m_commandList = nullptr;
        };

        void AsyncIOCallback(const IO::AsyncBlockReadResult& result) override;

        bool OnHeaderLoaded(festd::span<const std::byte> data, Request* request, IO::IStream* stream,
                            std::pmr::memory_resource* bufferAllocator);
        bool OnMipChainLoaded(const std::byte* data, Request* request, uint32_t mipChainIndex, const std::byte* dataToDelete,
                              std::pmr::memory_resource* bufferAllocator);

        Logger* m_logger;
        IJobSystem* m_jobSystem;
        IO::IAsyncStreamIO* m_asyncIO;
        Core::ResourcePool* m_resourcePool;
        Core::AsyncCopyQueue* m_asyncCopy;

        Threading::SpinLock m_lock;
        Memory::Pool<TextureAsset> m_assetPool{ "TextureAssetPool" };
        Memory::Pool<Request> m_requestPool{ "TextureRequestPool" };
        Memory::Pool<MipFinalizerJob> m_mipFinalizerJobPool{ "MipFinalizerJobPool" };

        static constexpr uint32_t kAsyncCopyCommandSegmentSize = 1024;
        Memory::SpinLockedPoolAllocator m_asyncCopyCommandPagePool{ "AsyncCopyCommandPagePool", kAsyncCopyCommandSegmentSize };
        Memory::SpinLockedPoolAllocator m_asyncCopyCommandListPool{ "AsyncCopyCommandListPool",
                                                                    sizeof(Core::AsyncCopyCommandList) };
    };
} // namespace FE::Graphics
