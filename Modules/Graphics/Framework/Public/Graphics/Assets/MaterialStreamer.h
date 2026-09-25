#pragma once
#include <Core/IO/AssetManager.h>
#include <Graphics/Assets/MaterialAssets.h>

namespace FE::Graphics::Core
{
    struct PipelineFactory;
} // namespace FE::Graphics::Core

namespace FE::Graphics
{
    struct MaterialParameterAllocator;

    struct MaterialStreamer final : public IO::Streamer
    {
        MaterialStreamer(MaterialParameterAllocator* allocator, Core::PipelineFactory* pipelineFactory);

        IO::AssetFinalizeResult FinalizeAssetLoading(IO::AssetSlot& assetSlot, const IO::ArtifactRecord& artifact,
                                                     void* candidate) override;
        IO::AssetFinalizeResult PollFinalize(IO::AssetSlot& assetSlot, void* candidate) override;
        void CancelFinalize(IO::AssetSlot& assetSlot, void* candidate) override;
        void OnAssetDestroyed(IO::AssetSlot& assetSlot, void* asset) override;
        bool RequiresFinalizedDependencies() const override;

    private:
        MaterialParameterAllocator* m_allocator = nullptr;
        Core::PipelineFactory* m_pipelineFactory = nullptr;
    };
} // namespace FE::Graphics
