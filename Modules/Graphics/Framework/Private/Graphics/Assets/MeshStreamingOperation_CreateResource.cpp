#include <Graphics/Assets/MeshStreamingOperation.h>

namespace FE::Graphics
{
    struct MeshStreamingOperation_CreateResource final : public MeshStreamingOperation
    {
        MeshStreamingOperation_CreateResource(const MeshAsset& asset, const IO::ArtifactRecord& artifact,
                                              const uint32_t targetLod, Core::Device* device, Core::ResourcePool* resourcePool,
                                              Core::AsyncCopyQueue* asyncCopyQueue)
            : MeshStreamingOperation(AssetStreamingOperationType::kCreateResource, asset, artifact, targetLod, device,
                                     resourcePool, asyncCopyQueue)
        {
            Start();
        }

        void Destroy() override
        {
            Memory::DefaultDelete(this);
        }
    };


    MeshStreamingOperation* CreateMeshStreamingOperation_CreateResource(const MeshAsset& asset,
                                                                        const IO::ArtifactRecord& artifact,
                                                                        const uint32_t targetLod, Core::Device* device,
                                                                        Core::ResourcePool* resourcePool,
                                                                        Core::AsyncCopyQueue* asyncCopyQueue)
    {
        return Memory::DefaultNew<MeshStreamingOperation_CreateResource>(asset,
                                                                         artifact,
                                                                         targetLod,
                                                                         device,
                                                                         resourcePool,
                                                                         asyncCopyQueue);
    }
} // namespace FE::Graphics
