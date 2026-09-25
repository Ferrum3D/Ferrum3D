#pragma once
#include <Core/IO/Artifact.h>
#include <Core/IO/AssetManager.h>
#include <Graphics/Assets/Assets.h>

namespace FE::Graphics::Core
{
    struct AsyncCopyQueue;
    struct DescriptorManager;
    struct Device;
    struct ResourcePool;
} // namespace FE::Graphics::Core

namespace FE::Graphics
{
    //! Explicit mip residency controller. Asset mip indices are least-detailed-first and request the inclusive range [0, mipIndex].
    struct TextureStreamer final : public IO::Streamer
    {
        TextureStreamer(Core::Device* device, Core::ResourcePool* resourcePool, Core::AsyncCopyQueue* asyncCopyQueue,
                        Core::DescriptorManager* descriptorManager);
        ~TextureStreamer() override;

        TextureStreamer(const TextureStreamer&) = delete;
        TextureStreamer& operator=(const TextureStreamer&) = delete;

        void SetResidentMip(const TextureAsset& asset, uint32_t mipIndex);

        IO::AssetFinalizeResult FinalizeAssetLoading(IO::AssetSlot& assetSlot, const IO::ArtifactRecord& artifact,
                                                     void* candidate) override;
        IO::AssetFinalizeResult PollFinalize(IO::AssetSlot& assetSlot, void* candidate) override;
        void CancelFinalize(IO::AssetSlot& assetSlot, void* candidate) override;
        void Tick() override;
        [[nodiscard]] bool HasRunningOperations(const IO::AssetSlot& assetSlot, const void* asset) const override;
        void OnAssetDestroyed(IO::AssetSlot& assetSlot, void* asset) override;
        void CancelAssetOperations(IO::AssetSlot& assetSlot, void* asset) override;

    private:
        struct Impl;
        Impl* m_impl = nullptr;
    };
} // namespace FE::Graphics
