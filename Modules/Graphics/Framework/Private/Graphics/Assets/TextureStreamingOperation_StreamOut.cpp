#include <Graphics/Assets/TextureStreamingOperation.h>

namespace FE::Graphics
{
    struct TextureStreamingOperation_StreamOut final : public TextureStreamingOperation
    {
        TextureStreamingOperation_StreamOut(const TextureAsset& asset, const IO::ArtifactRecord& artifact,
                                            const uint32_t targetMip, Core::Device* device, Core::ResourcePool* resourcePool,
                                            Core::AsyncCopyQueue* asyncCopyQueue)
            : TextureStreamingOperation(AssetStreamingOperationType::kStreamOut, asset, artifact, targetMip, device, resourcePool,
                                        asyncCopyQueue)
        {
            Start();
        }

        void Destroy() override
        {
            Memory::DefaultDelete(this);
        }
    };


    TextureStreamingOperation* CreateTextureStreamingOperation_StreamOut(const TextureAsset& asset,
                                                                         const IO::ArtifactRecord& artifact,
                                                                         const uint32_t targetMip, Core::Device* device,
                                                                         Core::ResourcePool* resourcePool,
                                                                         Core::AsyncCopyQueue* asyncCopyQueue)
    {
        return Memory::DefaultNew<TextureStreamingOperation_StreamOut>(asset,
                                                                       artifact,
                                                                       targetMip,
                                                                       device,
                                                                       resourcePool,
                                                                       asyncCopyQueue);
    }
} // namespace FE::Graphics
