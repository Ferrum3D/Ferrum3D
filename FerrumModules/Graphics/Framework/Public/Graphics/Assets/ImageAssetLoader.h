#pragma once
#include <FeCore/Assets/IAssetLoader.h>
#include <FeCore/IO/IAsyncStreamIO.h>
#include <FeCore/IO/IStreamFactory.h>
#include <FeCore/Jobs/Job.h>
#include <FeCore/Memory/PoolAllocator.h>

namespace FE::Graphics
{
    struct ImageAssetLoader final
        : public Assets::IAssetLoader
        , public IO::IAsyncReadCallback
    {
        FE_RTTI_Class(ImageAssetLoader, "20BA2066-1FC7-46E4-9708-B45CE3EE177C");

        ImageAssetLoader(IO::IStreamFactory* streamFactory, IO::IAsyncStreamIO* asyncIO, IJobSystem* jobSystem);
        ~ImageAssetLoader() override = default;

        const Assets::AssetLoaderSpec& GetSpec() const override;

        void CreateStorage(Assets::AssetStorage** ppStorage) override;

        void LoadAsset(Assets::AssetStorage* storage, Env::Name assetName) override;

    private:
        struct MipUploadJob;

        Memory::PoolAllocator m_uploadJobPool;

        festd::fixed_vector<festd::string_view, 2> m_sourceExtensions;
        Assets::AssetLoaderSpec m_spec;

        IO::IStreamFactory* m_streamFactory = nullptr;
        IO::IAsyncStreamIO* m_asyncStreamIO = nullptr;
        IJobSystem* m_jobSystem = nullptr;

        void AsyncIOCallback(const IO::AsyncReadResult& result) override;
    };
} // namespace FE::Graphics
