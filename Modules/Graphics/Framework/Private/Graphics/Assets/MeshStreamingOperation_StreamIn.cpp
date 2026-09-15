#include <Graphics/Assets/MeshStreamingOperation.h>

namespace FE::Graphics
{
    struct MeshStreamingOperation_StreamIn final : public MeshStreamingOperation
    {
        MeshStreamingOperation_StreamIn(const MeshAsset& asset, const IO::ArtifactRecord& artifact, const uint32_t targetLod,
                                        Core::Device* device, Core::ResourcePool* resourcePool,
                                        Core::AsyncCopyQueue* asyncCopyQueue)
            : MeshStreamingOperation(AssetStreamingOperationType::kStreamIn, asset, artifact, targetLod, device, resourcePool,
                                     asyncCopyQueue)
        {
            Start();
        }

        void Destroy() override
        {
            Memory::DefaultDelete(this);
        }
    };


    MeshStreamingOperation* CreateMeshStreamingOperation_StreamIn(const MeshAsset& asset, const IO::ArtifactRecord& artifact,
                                                                  const uint32_t targetLod, Core::Device* device,
                                                                  Core::ResourcePool* resourcePool,
                                                                  Core::AsyncCopyQueue* asyncCopyQueue)
    {
        return Memory::DefaultNew<MeshStreamingOperation_StreamIn>(asset,
                                                                   artifact,
                                                                   targetLod,
                                                                   device,
                                                                   resourcePool,
                                                                   asyncCopyQueue);
    }
} // namespace FE::Graphics
