#pragma once
#include <Core/IO/AssetManager.h>
#include <Graphics/Assets/ShaderAssets.h>

namespace FE::Graphics
{
    //! @brief Mounts published shader packages as shader source packages, replacing the previous generation on reload.
    struct ShaderPackageStreamer final : public IO::Streamer
    {
        IO::AssetFinalizeResult FinalizeAssetLoading(IO::AssetSlot& assetSlot, const IO::ArtifactRecord& artifact,
                                                     void* candidate) override;
        IO::AssetFinalizeResult PollFinalize(IO::AssetSlot& assetSlot, void* candidate) override;
        void CancelFinalize(IO::AssetSlot& assetSlot, void* candidate) override;
        void OnAssetDestroyed(IO::AssetSlot& assetSlot, void* asset) override;
    };


    //! @brief Makes the shaders of a published shader cache visible to shader compilers.
    struct ShaderCacheStreamer final : public IO::Streamer
    {
        IO::AssetFinalizeResult FinalizeAssetLoading(IO::AssetSlot& assetSlot, const IO::ArtifactRecord& artifact,
                                                     void* candidate) override;
        IO::AssetFinalizeResult PollFinalize(IO::AssetSlot& assetSlot, void* candidate) override;
        void CancelFinalize(IO::AssetSlot& assetSlot, void* candidate) override;
        void OnAssetDestroyed(IO::AssetSlot& assetSlot, void* asset) override;
    };
} // namespace FE::Graphics
