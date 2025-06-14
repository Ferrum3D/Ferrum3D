#pragma once
#include <FeCore/IO/IAsyncStreamIO.h>
#include <FeCore/Memory/PoolAllocator.h>
#include <Graphics/Assets/ITextureAssetManager.h>
#include <Graphics/Assets/TextureAssetFormat.h>
#include <Graphics/Core/Base/Limits.h>
#include <Graphics/Core/ResourcePool.h>
#include <festd/vector.h>

namespace FE::Graphics
{
    struct TextureAssetManager final
        : public ITextureAssetManager
        , public IO::IAsyncReadCallback
    {
        FE_RTTI_Class(TextureAssetManager, "9BC819E7-F2CD-47D3-A84C-421CEF60FC98");

        TextureAssetManager(Logger* logger, IO::IAsyncStreamIO* asyncIO, Core::ResourcePool* resourcePool);

        TextureAsset* Load(Env::Name assetName) override;

    private:
        enum class LoadingStage : uint32_t
        {
            kHeader,
            kMips,
        };

        struct Request final
        {
            Data::TextureHeader m_header;
            festd::fixed_vector<Data::MipChainInfo, Core::Limits::Image::kMaxMipCount> m_mipChains;
            Rc<TextureAsset> m_asset;
            LoadingStage m_stage;
        };

        void AsyncIOCallback(const IO::AsyncBlockReadResult& result) override;

        bool OnHeaderLoaded(festd::span<const std::byte> data, Request& request, IO::IStream* stream);
        bool OnMipChainLoaded(festd::span<const std::byte> data, Request& request, uint32_t mipChainIndex);

        Logger* m_logger;
        IO::IAsyncStreamIO* m_asyncIO;
        Core::ResourcePool* m_resourcePool;

        Threading::SpinLock m_lock;
        Memory::Pool<TextureAsset> m_assetPool{ "TextureAssetPool" };
        Memory::Pool<Request> m_requestPool{ "TextureRequestPool" };
    };
} // namespace FE::Graphics
