#include <FeCore/Logging/Trace.h>
#include <FeCore/Utils/BinarySerializer.h>
#include <Graphics/Assets/MeshAssetLoader.h>
#include <Graphics/Assets/MeshAssetStorage.h>
#include <Graphics/Assets/MeshLoaderImpl.h>
#include <Graphics/RHI/Buffer.h>
#include <Graphics/RHI/CommandList.h>
#include <Graphics/RHI/CommandQueue.h>
#include <Graphics/RHI/Fence.h>

namespace FE::Graphics
{
    MeshAssetLoader::MeshAssetLoader(IO::IStreamFactory* streamFactory)
        : m_streamFactory(streamFactory)
    {
        m_spec.AssetTypeName = Env::Name{ MeshAssetStorage::kAssetTypeName };
        m_spec.FileExtension = ".fbx";
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
            m_streamFactory->OpenFileStream(IO::FixedPath{ assetName } + ".fbx", IO::OpenMode::kReadOnly).Unwrap();

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

        Rc<RHI::Buffer> indexBufferStaging, vertexBufferStaging;
        {
            vertexBufferStaging = pServiceProvider->ResolveRequired<RHI::Buffer>();
            vertexBufferStaging->Init("Staging vertex", RHI::BufferDesc(vertexSize, RHI::BindFlags::kNone));
            vertexBufferStaging->AllocateMemory(RHI::MemoryType::kHostVisible);
            vertexBufferStaging->UpdateData(vertexData.data());

            meshStorage->m_vertexBuffer = pServiceProvider->ResolveRequired<RHI::Buffer>();
            meshStorage->m_vertexBuffer->Init("Vertex", RHI::BufferDesc(vertexSize, RHI::BindFlags::kVertexBuffer));
            meshStorage->m_vertexBuffer->AllocateMemory(RHI::MemoryType::kDeviceLocal);
        }
        {
            indexBufferStaging = pServiceProvider->ResolveRequired<RHI::Buffer>();
            indexBufferStaging->Init("Staging index", RHI::BufferDesc(indexSize, RHI::BindFlags::kNone));
            indexBufferStaging->AllocateMemory(RHI::MemoryType::kHostVisible);
            indexBufferStaging->UpdateData(indexData.data());

            meshStorage->m_indexBuffer = pServiceProvider->ResolveRequired<RHI::Buffer>();
            meshStorage->m_indexBuffer->Init("Index", RHI::BufferDesc(indexSize, RHI::BindFlags::kIndexBuffer));
            meshStorage->m_indexBuffer->AllocateMemory(RHI::MemoryType::kDeviceLocal);
        }

        const Rc commandList = pServiceProvider->ResolveRequired<RHI::CommandList>();
        commandList->Init({ RHI::HardwareQueueKindFlags::kTransfer, RHI::CommandListFlags::kOneTimeSubmit });
        commandList->Begin();

        RHI::BufferCopyRegion vertexCopyRegion{ vertexSize };
        RHI::BufferCopyRegion indexCopyRegion{ indexSize };

        commandList->CopyBuffers(vertexBufferStaging.Get(), meshStorage->m_vertexBuffer.Get(), vertexCopyRegion);
        commandList->CopyBuffers(indexBufferStaging.Get(), meshStorage->m_indexBuffer.Get(), indexCopyRegion);

        commandList->End();

        const Rc fence = pServiceProvider->ResolveRequired<RHI::Fence>();
        fence->Init();

        const Rc<RHI::Device> device = commandList->GetDevice();
        const Rc<RHI::CommandQueue> transferQueue = device->GetCommandQueue(RHI::HardwareQueueKindFlags::kTransfer);
        transferQueue->Execute(std::array{ commandList.Get() });
        transferQueue->SignalFence({ fence, 1 });
        fence->Wait(1);
    }
} // namespace FE::Graphics
