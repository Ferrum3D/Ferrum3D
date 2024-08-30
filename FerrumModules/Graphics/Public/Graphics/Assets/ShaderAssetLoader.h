#pragma once
#include <FeCore/Assets/IAssetLoader.h>
#include <FeCore/IO/IAsyncStreamIO.h>
#include <FeCore/IO/IStreamFactory.h>
#include <FeCore/Jobs/Job.h>
#include <FeCore/Memory/PoolAllocator.h>

namespace FE::Graphics
{
    class ShaderAssetLoader
        : public Assets::IAssetLoader
        , public IO::IAsyncReadCallback
    {
        struct CompilerJob;

        Memory::PoolAllocator m_CompilerJobPool;

        festd::fixed_vector<StringSlice, 2> m_SourceExtensions;
        Assets::AssetLoaderSpec m_Spec;

        IO::IStreamFactory* m_pStreamFactory = nullptr;
        IO::IAsyncStreamIO* m_pAsyncStreamIO = nullptr;
        IJobSystem* m_pJobSystem = nullptr;

        void AsyncIOCallback(const IO::AsyncReadResult& result) override;

    public:
        FE_RTTI_Class(ShaderAssetLoader, "004C59CC-768C-418F-99E1-D95D73F52444");

        ShaderAssetLoader(IO::IStreamFactory* pStreamFactory, IO::IAsyncStreamIO* pAsyncIO, IJobSystem* pJobSystem);
        ~ShaderAssetLoader() override = default;

        const Assets::AssetLoaderSpec& GetSpec() const override;

        void CreateStorage(Assets::AssetStorage** ppStorage) override;

        void LoadAsset(Assets::AssetStorage* storage, Env::Name assetName) override;
    };
} // namespace FE::Graphics
