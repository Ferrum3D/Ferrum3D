#include <Graphics/Assets/MeshStreamingOperation.h>

namespace FE::Graphics
{
    struct MeshStreamingOperation_StreamOut final : public MeshStreamingOperation
    {
        MeshStreamingOperation_StreamOut(const MeshAsset& asset, const IO::ArtifactRecord& artifact, const uint32_t targetLod,
                                         Core::Device* device, Core::ResourcePool* resourcePool,
                                         Core::AsyncCopyQueue* asyncCopyQueue)
            : MeshStreamingOperation(AssetStreamingOperationType::kStreamOut, asset, artifact, targetLod, device, resourcePool,
                                     asyncCopyQueue)
        {
            Start();
        }

        void Destroy() override
        {
            Memory::DefaultDelete(this);
        }
    };


    MeshStreamingOperation* CreateMeshStreamingOperation_StreamOut(const MeshAsset& asset, const IO::ArtifactRecord& artifact,
                                                                   const uint32_t targetLod, Core::Device* device,
                                                                   Core::ResourcePool* resourcePool,
                                                                   Core::AsyncCopyQueue* asyncCopyQueue)
    {
        return Memory::DefaultNew<MeshStreamingOperation_StreamOut>(asset,
                                                                    artifact,
                                                                    targetLod,
                                                                    device,
                                                                    resourcePool,
                                                                    asyncCopyQueue);
    }
} // namespace FE::Graphics
