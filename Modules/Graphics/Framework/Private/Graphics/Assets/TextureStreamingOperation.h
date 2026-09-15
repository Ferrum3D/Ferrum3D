#pragma once
#include <Core/Memory/PoolAllocator.h>
#include <Graphics/Assets/AssetStreamingOperation.h>
#include <Graphics/Assets/Assets.h>
#include <Graphics/Core/Device.h>
#include <Graphics/Core/ResourcePool.h>

namespace FE::Graphics
{
    struct TextureStreamingOperation : public AssetStreamingOperation
    {
        AssetStreamingOperationStatus Tick() final;
        void Cancel() final;

        Rc<Core::Texture> TakeTexture();

    protected:
        TextureStreamingOperation(AssetStreamingOperationType type, const TextureAsset& asset, const IO::ArtifactRecord& artifact,
                                  uint32_t targetMip, Core::Device* device, Core::ResourcePool* resourcePool,
                                  Core::AsyncCopyQueue* asyncCopyQueue);

        void Start();

    private:
        void Prepare();

        const TextureAsset& m_asset;
        IO::ArtifactRecord m_artifact;
        Core::Device* m_device = nullptr;
        Core::ResourcePool* m_resourcePool = nullptr;
        Memory::PoolAllocator m_commandAllocator;
        Rc<Core::Texture> m_texture;
    };


    TextureStreamingOperation* CreateTextureStreamingOperation_CreateResource(const TextureAsset& asset,
                                                                              const IO::ArtifactRecord& artifact,
                                                                              uint32_t targetMip, Core::Device* device,
                                                                              Core::ResourcePool* resourcePool,
                                                                              Core::AsyncCopyQueue* asyncCopyQueue);
    TextureStreamingOperation* CreateTextureStreamingOperation_StreamIn(const TextureAsset& asset,
                                                                        const IO::ArtifactRecord& artifact, uint32_t targetMip,
                                                                        Core::Device* device, Core::ResourcePool* resourcePool,
                                                                        Core::AsyncCopyQueue* asyncCopyQueue);
    TextureStreamingOperation* CreateTextureStreamingOperation_StreamOut(const TextureAsset& asset,
                                                                         const IO::ArtifactRecord& artifact, uint32_t targetMip,
                                                                         Core::Device* device, Core::ResourcePool* resourcePool,
                                                                         Core::AsyncCopyQueue* asyncCopyQueue);
} // namespace FE::Graphics
