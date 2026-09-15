#include <Graphics/Assets/TextureStreamingOperation.h>

namespace FE::Graphics
{
    struct TextureStreamingOperation_CreateResource final : public TextureStreamingOperation
    {
        TextureStreamingOperation_CreateResource(const TextureAsset& asset, const IO::ArtifactRecord& artifact,
                                                 const uint32_t targetMip, Core::Device* device, Core::ResourcePool* resourcePool,
                                                 Core::AsyncCopyQueue* asyncCopyQueue)
            : TextureStreamingOperation(AssetStreamingOperationType::kCreateResource, asset, artifact, targetMip, device,
                                        resourcePool, asyncCopyQueue)
        {
            Start();
        }

        void Destroy() override
        {
            Memory::DefaultDelete(this);
        }
    };


    TextureStreamingOperation* CreateTextureStreamingOperation_CreateResource(const TextureAsset& asset,
                                                                              const IO::ArtifactRecord& artifact,
                                                                              const uint32_t targetMip, Core::Device* device,
                                                                              Core::ResourcePool* resourcePool,
                                                                              Core::AsyncCopyQueue* asyncCopyQueue)
    {
        return Memory::DefaultNew<TextureStreamingOperation_CreateResource>(asset,
                                                                            artifact,
                                                                            targetMip,
                                                                            device,
                                                                            resourcePool,
                                                                            asyncCopyQueue);
    }
} // namespace FE::Graphics
