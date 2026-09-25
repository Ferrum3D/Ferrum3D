#include <Core/IO/Artifact.h>
#include <Core/IO/AssetManager.h>
#include <Core/Jobs/Jobs.h>
#include <Core/Math/Matrix4x4.h>
#include <Core/Threading/Thread.h>
#include <Framework/Application/Application.h>
#include <Graphics/Assets/MaterialAssets.h>
#include <Graphics/Assets/Streamers.h>
#include <Graphics/Core/Device.h>
#include <Graphics/Core/DeviceFactory.h>
#include <Graphics/Core/PipelineFactory.h>
#include <Graphics/Core/PipelineVariantSet.h>
#include <Graphics/Core/Viewport.h>
#include <Graphics/Features/Mesh/MeshSceneModule.h>
#include <Graphics/Materials/MaterialInstance.h>
#include <Graphics/Passes/DepthPrepass.h>
#include <Graphics/Passes/DrawTags.h>
#include <Graphics/Passes/OpaquePass.h>
#include <Graphics/Renderer.h>

using namespace FE;
using namespace FE::Graphics;

inline constexpr const char* kExampleName = "Ferrum3D - Renderer Sample";
const IO::AssetID kBunnyModelAssetId("4B01B878-5EEF-4D6A-AD04-FC9AF1F78416");
const IO::AssetID kBunnyMaterialAssetId("EFA88960-2315-407E-BAAC-F88DFDF6FFBC");

namespace
{
    struct ExampleApplication final : public Framework::Application
    {
        ~ExampleApplication() override
        {
            m_device->WaitIdle();

            m_scene.Reset();
            m_view.Reset();
            m_viewport.Reset();

            m_materialInstance.Reset();
            m_materialRequest.Reset();

            m_mesh.Reset();
            m_modelRequest.Reset();
            IO::AssetManager::Shutdown();

            Memory::DefaultDelete(m_materialStreamer);
            Memory::DefaultDelete(m_textureStreamer);
            Memory::DefaultDelete(m_meshStreamer);

            m_pipelineFactory.Reset();
            m_device.Reset();
            Renderer::Shutdown();
            Core::DeviceFactory::Shutdown();
        }

        void InitializeApp()
        {
            FE_PROFILER_ZONE();

            Core::DeviceFactory::Init(Core::GraphicsApi::kVulkan);

            auto& factory = Core::DeviceFactory::Get();
            for (const Core::AdapterInfo& adapterInfo : factory.EnumerateAdapters())
            {
                if (adapterInfo.m_kind == Core::AdapterKind::kDiscrete)
                {
                    m_device = factory.CreateDevice(adapterInfo.m_name);
                    break;
                }
            }

            Renderer::Init(m_device.Get());

            auto& renderer = Renderer::Get();
            Core::ResourcePool* resourcePool = renderer.GetResourcePool();
            Core::GraphicsQueue* graphicsQueue = renderer.GetGraphicsQueue();

            IO::ArtifactStore::SetCatalogSource(FE_RENDERER_SAMPLE_ASSET_DIR);
            IO::AssetManager::Init();
            m_meshStreamer = Memory::DefaultNew<MeshStreamer>(m_device.Get(), resourcePool, renderer.GetAsyncCopyQueue());
            m_textureStreamer = Memory::DefaultNew<TextureStreamer>(m_device.Get(),
                                                                    resourcePool,
                                                                    renderer.GetAsyncCopyQueue(),
                                                                    renderer.GetDescriptorManager());
            IO::AssetManager::RegisterStreamer(Rtti::GetTypeID<MeshAsset>(), m_meshStreamer);
            IO::AssetManager::RegisterStreamer(Rtti::GetTypeID<TextureAsset>(), m_textureStreamer);

            const RectInt clientRect = m_mainWindow->GetClientRect();
            m_viewport = m_device->CreateViewport(resourcePool, graphicsQueue);
            m_viewport->Init(Core::ViewportDesc::Create(clientRect, m_mainWindow->GetNativeHandle().m_value));

            m_pipelineFactory = m_device->CreatePipelineFactory(renderer.GetDescriptorManager());
            m_scene = renderer.CreateScene();
            m_materialStreamer =
                Memory::DefaultNew<MaterialStreamer>(renderer.GetMaterialParameterAllocator(), m_pipelineFactory.Get());
            IO::AssetManager::RegisterStreamer(Rtti::GetTypeID<MaterialInstanceAsset>(), m_materialStreamer);
            Core::CompileGlobalPipelineSets(m_pipelineFactory.Get());
            Core::WaitForGlobalPipelineSets();

            m_scene->GetModules().Add<MeshSceneModule>();
            m_view = m_scene->CreateView();
            m_view->GetModules().Add<DepthPrepass::ViewModule>();
            m_view->GetModules().Add<OpaquePass::ViewModule>();

            const float aspectRatio = static_cast<float>(clientRect.Width()) / static_cast<float>(clientRect.Height());
            const Vector3 cameraPosition(0.0f, 2.0f, -2.5f);
            const Vector3 cameraTarget(0.0f, 0.75f, 0.0f);
            const Matrix4x4 cameraMatrix = Math::Invert(Matrix4x4::LookAt(cameraPosition, cameraTarget, Vector3::AxisY()));
            m_view->SetCameraTransform(Transform::Create(cameraPosition, Math::ExtractRotation(cameraMatrix), 1.0f));
            m_view->SetProjection(Constants::kPI * 0.3f, aspectRatio, 0.01f, 1000.0f);

            const IO::Link<ModelAsset> modelLink(kBunnyModelAssetId);
            m_modelRequest = IO::AssetManager::LoadAsset(modelLink);
            while (!m_modelRequest.IsCompleted())
            {
                IO::AssetManager::Tick();
                Threading::Sleep(1);
            }
            FE_Assert(m_modelRequest.GetResult() == IO::AssetLoadResult::kSucceeded, "Failed to load bunny model asset");

            const IO::AssetHandle<ModelAsset> modelHandle(m_modelRequest.GetAssetSlot());
            const IO::AssetRead<ModelAsset> model = modelHandle.Read();
            FE_Assert(model && model->m_meshes.size() == 1);
            m_mesh = model->m_meshes[0].GetAssetHandle().Read();
            FE_Assert(m_mesh);

            m_materialRequest = IO::AssetManager::LoadAsset(IO::Link<MaterialInstanceAsset>(kBunnyMaterialAssetId));
            while (!m_materialRequest.IsCompleted())
            {
                IO::AssetManager::Tick();
                Threading::Sleep(1);
            }

            FE_Assert(m_materialRequest.GetResult() == IO::AssetLoadResult::kSucceeded, "Failed to load bunny material instance");
            m_materialInstance = IO::AssetHandle<MaterialInstanceAsset>(m_materialRequest.GetAssetSlot()).Read();
            FE_Assert(m_materialInstance && m_materialInstance->m_runtime);
            for (const MaterialParameterValue& parameter : m_materialInstance->m_parameters)
            {
                if (!parameter.m_texture.GetAssetID().IsValid())
                    continue;

                const IO::AssetRead<TextureAsset> texture = parameter.m_texture.GetAssetHandle().Read();
                FE_Assert(texture);
                m_textureStreamer->SetResidentMip(*texture.Get(), 1);
            }
            m_instanceParameters = m_materialInstance->m_runtime->AllocateInstanceParameters();

            auto& meshSceneModule = m_scene->GetModules().Find<MeshSceneModule>();

            MeshBatchDesc batchDesc;
            batchDesc.m_bounds = Aabb{ Vector3(-10.0f, -10.0f, -10.0f), Vector3(10.0f, 10.0f, 10.0f) };
            batchDesc.m_drawTagMask = DrawTagMask(DrawTags::DepthPrepass) | DrawTagMask(DrawTags::Opaque);
            m_batch = meshSceneModule.CreateBatch(batchDesc);

            MeshInstanceDesc instanceDesc;
            instanceDesc.m_asset = m_mesh.Get();
            instanceDesc.m_batch = m_batch;
            instanceDesc.m_material = m_materialInstance->m_runtime;
            instanceDesc.m_instanceData = m_instanceParameters.m_devicePointer;
            instanceDesc.m_transform = Matrix4x4::RotationY(Constants::kPI);

            m_meshInstance = meshSceneModule.CreateInstance(instanceDesc);
        }

        Rc<WaitGroup> ScheduleUpdate() override
        {
            FE_PROFILER_ZONE();
            IO::AssetManager::Tick();
            Renderer::Get().Render(m_scene.Get(), m_viewport.Get());
            return nullptr;
        }

        void DoRelease() override
        {
            this->~ExampleApplication();
        }

        Rc<Core::Device> m_device;
        Rc<Core::Viewport> m_viewport;
        Rc<Core::PipelineFactory> m_pipelineFactory;

        Rc<Scene> m_scene;
        Rc<View> m_view;

        IO::AssetRequest m_modelRequest;
        IO::AssetRequest m_materialRequest;
        IO::AssetRead<MeshAsset> m_mesh;
        IO::AssetRead<MaterialInstanceAsset> m_materialInstance;
        MaterialParameterAllocator::Allocation m_instanceParameters;
        MaterialStreamer* m_materialStreamer = nullptr;
        MeshStreamer* m_meshStreamer = nullptr;
        TextureStreamer* m_textureStreamer = nullptr;
        MeshBatch* m_batch = nullptr;
        MeshHandle m_meshInstance;
    };
} // namespace

int main(const int32_t argc, const char** argv)
{
    Env::ApplicationInfo applicationInfo;
    applicationInfo.m_name = kExampleName;

    Env::Init(applicationInfo, argc, argv);

    std::pmr::memory_resource* allocator = Env::GetStaticAllocator(Memory::StaticAllocatorType::kLinear);
    auto* application = Memory::New<ExampleApplication>(allocator);

    int32_t exitCode = 0;
    Jobs::DispatchMainThread([application, &exitCode] {
        application->InitializeWindow();
        application->InitializeApp();
        exitCode = application->Run();
    });

    Jobs::StartJobSystem();

    Memory::Delete(allocator, application);
    Env::Shutdown();
    return exitCode;
}
