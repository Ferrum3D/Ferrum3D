#pragma once
#include <FeCore/Assets/IAssetLoader.h>
#include <FeCore/IO/IAsyncStreamIO.h>
#include <FeCore/IO/IStreamFactory.h>
#include <FeCore/Jobs/Job.h>
#include <FeCore/Memory/PoolAllocator.h>

namespace FE::Graphics
{
    struct ShaderAssetLoader final
        : public Assets::IAssetLoader
        , public IO::IAsyncReadCallback
    {
        FE_RTTI_Class(ShaderAssetLoader, "004C59CC-768C-418F-99E1-D95D73F52444");

        ShaderAssetLoader(IO::IStreamFactory* pStreamFactory, IO::IAsyncStreamIO* pAsyncIO, IJobSystem* pJobSystem);
        ~ShaderAssetLoader() override = default;

        const Assets::AssetLoaderSpec& GetSpec() const override;

        void CreateStorage(Assets::AssetStorage** ppStorage) override;

        void LoadAsset(Assets::AssetStorage* storage, Env::Name assetName) override;

    private:
        struct CompilerJob;

        Memory::PoolAllocator m_compilerJobPool;

        festd::fixed_vector<StringSlice, 2> m_sourceExtensions;
        Assets::AssetLoaderSpec m_spec;

        IO::IStreamFactory* m_streamFactory = nullptr;
        IO::IAsyncStreamIO* m_asyncStreamIO = nullptr;
        IJobSystem* m_jobSystem = nullptr;

        void AsyncIOCallback(const IO::AsyncReadResult& result) override;
    };
} // namespace FE::Graphics
