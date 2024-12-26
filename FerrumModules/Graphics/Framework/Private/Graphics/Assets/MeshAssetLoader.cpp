#include <FeCore/Logging/Trace.h>
#include <FeCore/Utils/BinarySerializer.h>
#include <Graphics/Assets/MeshAssetLoader.h>
#include <Graphics/Assets/MeshAssetStorage.h>
#include <Graphics/Assets/MeshLoaderImpl.h>
#include <Graphics/RHI/CommandList.h>
#include <Graphics/RHI/CommandQueue.h>
#include <Graphics/RHI/Fence.h>
#include <Graphics/RHI/ResourcePool.h>

namespace FE::Graphics
{
    MeshAssetLoader::MeshAssetLoader(IO::IStreamFactory* streamFactory)
        : m_streamFactory(streamFactory)
    {
        m_spec.m_assetTypeName = Env::Name{ MeshAssetStorage::kAssetTypeName };
        m_spec.m_fileExtension = ".fbx";
    }


    const Assets::AssetLoaderSpec& MeshAssetLoader::GetSpec() const
    {
        return m_spec;
    }


    void MeshAssetLoader::CreateStorage(Assets::AssetStorage** ppStorage)
    {
        *ppStorage = Rc<MeshAssetStorage>::DefaultNew(this);
        (*ppStorage)->AddStrongRef();
    }


    void MeshAssetLoader::LoadAsset(Assets::AssetStorage* storage, Env::Name assetName)
    {
        ZoneScoped;

        const Rc assetStream =
            m_streamFactory->OpenFileStream(IO::FixedPath{ assetName } + ".fbx", IO::OpenMode::kReadOnly).value();

        festd::vector<std::byte> buffer(static_cast<uint32_t>(assetStream->Length()), 0);
        assetStream->ReadToBuffer(buffer);

        uint32_t vertexCount;
        festd::vector<float> vertexData;
        festd::vector<uint32_t> indexData;
        std::array components{ MeshVertexComponent::Position3F, MeshVertexComponent::TextureCoordinate2F };
        const bool result = LoadMeshFromMemory(buffer, components, vertexData, indexData, vertexCount);
        FE_AssertMsg(result, "Failed to load a mesh");

        auto* meshStorage = static_cast<MeshAssetStorage*>(storage);

        const uint32_t vertexSize = vertexData.size() * sizeof(vertexData[0]);
        const uint32_t indexSize = indexData.size() * sizeof(indexData[0]);

        DI::IServiceProvider* pServiceProvider = Env::GetServiceProvider();
        auto* resourcePool = pServiceProvider->ResolveRequired<RHI::ResourcePool>();

        Rc<RHI::Buffer> indexBufferStaging, vertexBufferStaging;
        {
            auto desc = RHI::BufferDesc(vertexSize, RHI::BindFlags::kNone, RHI::ResourceUsage::kHostWriteThrough);
            vertexBufferStaging = resourcePool->CreateBuffer("Staging vertex", desc).value();
            vertexBufferStaging->UpdateData(vertexData.data());

            desc.m_usage = RHI::ResourceUsage::kDeviceOnly;
            desc.m_flags = RHI::BindFlags::kVertexBuffer;
            meshStorage->m_vertexBuffer = resourcePool->CreateBuffer("Vertex", desc).value();
        }
        {
            auto desc = RHI::BufferDesc(indexSize, RHI::BindFlags::kNone, RHI::ResourceUsage::kHostWriteThrough);
            indexBufferStaging = resourcePool->CreateBuffer("Staging index", desc).value();
            indexBufferStaging->UpdateData(indexData.data());

            desc.m_usage = RHI::ResourceUsage::kDeviceOnly;
            desc.m_flags = RHI::BindFlags::kIndexBuffer;
            meshStorage->m_indexBuffer = resourcePool->CreateBuffer("Index", desc).value();
        }

        const Rc commandList = pServiceProvider->ResolveRequired<RHI::CommandList>();
        commandList->Init({ RHI::HardwareQueueKindFlags::kTransfer, RHI::CommandListFlags::kOneTimeSubmit });
        commandList->Begin();

        const RHI::BufferCopyRegion vertexCopyRegion{ vertexSize };
        const RHI::BufferCopyRegion indexCopyRegion{ indexSize };

        commandList->CopyBuffers(vertexBufferStaging.Get(), meshStorage->m_vertexBuffer.Get(), vertexCopyRegion);
        commandList->CopyBuffers(indexBufferStaging.Get(), meshStorage->m_indexBuffer.Get(), indexCopyRegion);

        commandList->End();

        const Rc fence = pServiceProvider->ResolveRequired<RHI::Fence>();
        fence->Init();

        RHI::Device* device = commandList->GetDevice();
        const Rc<RHI::CommandQueue> transferQueue = device->GetCommandQueue(RHI::HardwareQueueKindFlags::kTransfer);
        transferQueue->Execute(std::array{ commandList.Get() });
        transferQueue->SignalFence({ fence, 1 });
        fence->Wait(1);
    }
} // namespace FE::Graphics
