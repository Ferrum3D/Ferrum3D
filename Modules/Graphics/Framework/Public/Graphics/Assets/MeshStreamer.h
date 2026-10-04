#pragma once
#include <Core/IO/Artifact.h>
#include <Core/IO/AssetManager.h>
#include <Graphics/Assets/Assets.h>

namespace FE::Graphics::Core
{
    struct AsyncCopyQueue;
    struct Device;
    struct GraphicsQueue;
    struct ResourcePool;
} // namespace FE::Graphics::Core

namespace FE::Graphics
{
    //! Explicit LOD residency controller. Requests use finest-first indices: LOD 0 is the original mesh.
    struct MeshStreamer final : public IO::Streamer
    {
        MeshStreamer(Core::Device* device, Core::ResourcePool* resourcePool, Core::AsyncCopyQueue* asyncCopyQueue,
                     Core::GraphicsQueue* graphicsQueue);
        ~MeshStreamer() override;

        MeshStreamer(const MeshStreamer&) = delete;
        MeshStreamer& operator=(const MeshStreamer&) = delete;

        //! Makes the requested LOD and all coarser LODs resident. Asset residency indices remain least-detailed-first.
        void SetResidentLod(const MeshAsset& asset, uint32_t lodIndex);

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
