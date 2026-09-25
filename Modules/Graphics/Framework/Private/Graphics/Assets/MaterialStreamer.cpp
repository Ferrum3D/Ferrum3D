#include <Graphics/Assets/MaterialStreamer.h>
#include <Graphics/Materials/MaterialInstance.h>

namespace FE::Graphics
{
    MaterialStreamer::MaterialStreamer(MaterialParameterAllocator* allocator, Core::PipelineFactory* pipelineFactory)
        : m_allocator(allocator)
        , m_pipelineFactory(pipelineFactory)
    {
    }


    IO::AssetFinalizeResult MaterialStreamer::FinalizeAssetLoading(IO::AssetSlot&, const IO::ArtifactRecord&, void* candidate)
    {
        auto* asset = static_cast<MaterialInstanceAsset*>(candidate);
        FE_Assert(asset->m_runtime == nullptr);

        const IO::AssetRead<MaterialAsset> material = asset->m_material.GetAssetHandle().Read();
        if (!material)
            return IO::AssetFinalizeResult::kFailed;

        asset->m_runtime = Memory::DefaultNew<MaterialInstanceRuntime>(material.Get(), asset, m_allocator, m_pipelineFactory);
        return IO::AssetFinalizeResult::kSucceeded;
    }


    IO::AssetFinalizeResult MaterialStreamer::PollFinalize(IO::AssetSlot&, void* candidate)
    {
        return static_cast<MaterialInstanceAsset*>(candidate)->m_runtime ? IO::AssetFinalizeResult::kSucceeded
                                                                         : IO::AssetFinalizeResult::kFailed;
    }


    void MaterialStreamer::CancelFinalize(IO::AssetSlot&, void* candidate)
    {
        auto* asset = static_cast<MaterialInstanceAsset*>(candidate);
        Memory::DefaultDelete(asset->m_runtime);
        asset->m_runtime = nullptr;
    }


    void MaterialStreamer::OnAssetDestroyed(IO::AssetSlot& slot, void* asset)
    {
        CancelFinalize(slot, asset);
    }


    bool MaterialStreamer::RequiresFinalizedDependencies() const
    {
        return true;
    }
} // namespace FE::Graphics
