#include <FeCore/Logging/Trace.h>
#include <FeCore/Utils/BinarySerializer.h>
#include <Graphics/Assets/MeshAssetLoader.h>
#include <Graphics/Assets/MeshAssetStorage.h>
#include <Graphics/Assets/MeshLoaderImpl.h>
#include <HAL/Buffer.h>
#include <HAL/CommandList.h>
#include <HAL/CommandQueue.h>
#include <HAL/Fence.h>

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
        auto assetStream = m_streamFactory->OpenFileStream(IO::FixedPath{ assetName } + ".fbx", IO::OpenMode::kReadOnly).Unwrap();

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

        Rc<HAL::Buffer> indexBufferStaging, vertexBufferStaging;
        {
            vertexBufferStaging = pServiceProvider->ResolveRequired<HAL::Buffer>();
            vertexBufferStaging->Init("Staging vertex", HAL::BufferDesc(vertexSize, HAL::BindFlags::None));
            vertexBufferStaging->AllocateMemory(HAL::MemoryType::kHostVisible);
            vertexBufferStaging->UpdateData(vertexData.data());

            meshStorage->m_vertexBuffer = pServiceProvider->ResolveRequired<HAL::Buffer>();
            meshStorage->m_vertexBuffer->Init("Vertex", HAL::BufferDesc(vertexSize, HAL::BindFlags::VertexBuffer));
            meshStorage->m_vertexBuffer->AllocateMemory(HAL::MemoryType::kDeviceLocal);
        }
        {
            indexBufferStaging = pServiceProvider->ResolveRequired<HAL::Buffer>();
            indexBufferStaging->Init("Staging index", HAL::BufferDesc(indexSize, HAL::BindFlags::None));
            indexBufferStaging->AllocateMemory(HAL::MemoryType::kHostVisible);
            indexBufferStaging->UpdateData(indexData.data());

            meshStorage->m_indexBuffer = pServiceProvider->ResolveRequired<HAL::Buffer>();
            meshStorage->m_indexBuffer->Init("Index", HAL::BufferDesc(indexSize, HAL::BindFlags::IndexBuffer));
            meshStorage->m_indexBuffer->AllocateMemory(HAL::MemoryType::kDeviceLocal);
        }

        Rc commandList = pServiceProvider->ResolveRequired<HAL::CommandList>();
        commandList->Init({ HAL::HardwareQueueKindFlags::kTransfer, HAL::CommandListFlags::OneTimeSubmit });
        commandList->Begin();

        HAL::BufferCopyRegion vertexCopyRegion{ vertexSize };
        HAL::BufferCopyRegion indexCopyRegion{ indexSize };

        commandList->CopyBuffers(vertexBufferStaging.Get(), meshStorage->m_vertexBuffer.Get(), vertexCopyRegion);
        commandList->CopyBuffers(indexBufferStaging.Get(), meshStorage->m_indexBuffer.Get(), indexCopyRegion);

        commandList->End();
    }
} // namespace FE::Graphics
