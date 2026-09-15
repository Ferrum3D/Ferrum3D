#pragma once
#include <Core/Memory/PoolAllocator.h>
#include <Graphics/Assets/AssetStreamingOperation.h>
#include <Graphics/Assets/Assets.h>
#include <Graphics/Core/Device.h>
#include <Graphics/Core/ResourcePool.h>

namespace FE::Graphics
{
    struct MeshStreamingOperation : public AssetStreamingOperation
    {
        AssetStreamingOperationStatus Tick() final;
        void Cancel() final;

        Rc<Core::Buffer> TakeBuffer();

    protected:
        MeshStreamingOperation(AssetStreamingOperationType type, const MeshAsset& asset, const IO::ArtifactRecord& artifact,
                               uint32_t targetLod, Core::Device* device, Core::ResourcePool* resourcePool,
                               Core::AsyncCopyQueue* asyncCopyQueue);

        void Start();

    private:
        void Prepare();

        const MeshAsset& m_asset;
        IO::ArtifactRecord m_artifact;
        Core::Device* m_device = nullptr;
        Core::ResourcePool* m_resourcePool = nullptr;
        Memory::PoolAllocator m_commandAllocator;
        Rc<Core::Buffer> m_buffer;
    };


    MeshStreamingOperation* CreateMeshStreamingOperation_CreateResource(const MeshAsset& asset,
                                                                        const IO::ArtifactRecord& artifact, uint32_t targetLod,
                                                                        Core::Device* device, Core::ResourcePool* resourcePool,
                                                                        Core::AsyncCopyQueue* asyncCopyQueue);
    MeshStreamingOperation* CreateMeshStreamingOperation_StreamIn(const MeshAsset& asset, const IO::ArtifactRecord& artifact,
                                                                  uint32_t targetLod, Core::Device* device,
                                                                  Core::ResourcePool* resourcePool,
                                                                  Core::AsyncCopyQueue* asyncCopyQueue);
    MeshStreamingOperation* CreateMeshStreamingOperation_StreamOut(const MeshAsset& asset, const IO::ArtifactRecord& artifact,
                                                                   uint32_t targetLod, Core::Device* device,
                                                                   Core::ResourcePool* resourcePool,
                                                                   Core::AsyncCopyQueue* asyncCopyQueue);
} // namespace FE::Graphics
