#pragma once
#include <FeCore/Assets/IAssetLoader.h>
#include <FeCore/IO/IAsyncStreamIO.h>
#include <FeCore/IO/IStreamFactory.h>
#include <FeCore/Jobs/Job.h>
#include <FeCore/Memory/PoolAllocator.h>

namespace FE::Graphics
{
    class ImageAssetLoader
        : public Assets::IAssetLoader
        , public IO::IAsyncReadCallback
    {
        struct MipUploadJob;

        Memory::PoolAllocator m_UploadJobPool;

        festd::fixed_vector<StringSlice, 2> m_SourceExtensions;
        Assets::AssetLoaderSpec m_Spec;

        IO::IStreamFactory* m_pStreamFactory = nullptr;
        IO::IAsyncStreamIO* m_pAsyncStreamIO = nullptr;
        IJobSystem* m_pJobSystem = nullptr;

        void AsyncIOCallback(const IO::AsyncReadResult& result) override;

    public:
        FE_RTTI_Class(ImageAssetLoader, "20BA2066-1FC7-46E4-9708-B45CE3EE177C");

        ImageAssetLoader(IO::IStreamFactory* pStreamFactory, IO::IAsyncStreamIO* pAsyncIO, IJobSystem* pJobSystem);
        ~ImageAssetLoader() override = default;

        const Assets::AssetLoaderSpec& GetSpec() const override;

        void CreateStorage(Assets::AssetStorage** ppStorage) override;

        void LoadAsset(Assets::AssetStorage* storage, Env::Name assetName) override;
    };
} // namespace FE::Graphics
