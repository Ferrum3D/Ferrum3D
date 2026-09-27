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
const IO::AssetID kWarmBunnyMaterialAssetId("D5BA9140-9F93-4407-B5CE-E9FB01C2DDEC");
const IO::AssetID kCoolBunnyMaterialAssetId("FBD2877D-D6CB-4A11-A8A6-569E066A68EF");

namespace
{
    struct ExampleApplication final : public Framework::Application
    {
        ~ExampleApplication() override
        {
            m_device->WaitIdle();

            m_scene->GetModules().Remove<MeshSceneModule>();
            m_view.Reset();
            m_scene.Reset();
            m_viewport.Reset();

            m_materialRequest.Reset();
            m_warmMaterialRequest.Reset();
            m_coolMaterialRequest.Reset();

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
            m_meshStreamer =
                Memory::DefaultNew<MeshStreamer>(m_device.Get(), resourcePool, renderer.GetAsyncCopyQueue(), graphicsQueue);
            m_textureStreamer = Memory::DefaultNew<TextureStreamer>(m_device.Get(),
                                                                    resourcePool,
                                                                    renderer.GetAsyncCopyQueue(),
                                                                    graphicsQueue,
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
            const Vector3 cameraPosition(0.0f, 3.0f, -8.0f);
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
            const IO::AssetLease<MeshAsset> meshLease(model->m_meshes[0].GetAssetHandle().GetAssetSlot());
            FE_Assert(meshLease.IsReady());

            m_materialRequest = IO::AssetManager::LoadAsset(IO::Link<MaterialInstanceAsset>(kBunnyMaterialAssetId));
            m_warmMaterialRequest = IO::AssetManager::LoadAsset(IO::Link<MaterialInstanceAsset>(kWarmBunnyMaterialAssetId));
            m_coolMaterialRequest = IO::AssetManager::LoadAsset(IO::Link<MaterialInstanceAsset>(kCoolBunnyMaterialAssetId));
            while (!m_materialRequest.IsCompleted() || !m_warmMaterialRequest.IsCompleted()
                   || !m_coolMaterialRequest.IsCompleted())
            {
                IO::AssetManager::Tick();
                Threading::Sleep(1);
            }

            FE_Assert(m_materialRequest.GetResult() == IO::AssetLoadResult::kSucceeded, "Failed to load bunny material instance");
            FE_Assert(m_warmMaterialRequest.GetResult() == IO::AssetLoadResult::kSucceeded,
                      "Failed to load warm bunny material instance");
            FE_Assert(m_coolMaterialRequest.GetResult() == IO::AssetLoadResult::kSucceeded,
                      "Failed to load cool bunny material instance");
            const IO::AssetRead<MaterialInstanceAsset> materialInstance =
                IO::AssetHandle<MaterialInstanceAsset>(m_materialRequest.GetAssetSlot()).Read();
            FE_Assert(materialInstance && materialInstance->m_runtime);
            for (const MaterialParameterValue& parameter : materialInstance->m_parameters)
            {
                if (!parameter.m_texture.GetAssetID().IsValid())
                    continue;

                const IO::AssetRead<TextureAsset> texture = parameter.m_texture.GetAssetHandle().Read();
                FE_Assert(texture);
                m_textureStreamer->SetResidentMip(*texture.Get(), 1);
            }
            auto& meshSceneModule = m_scene->GetModules().Find<MeshSceneModule>();

            MeshInstanceDesc instanceDesc;
            instanceDesc.m_asset = meshLease;
            const IO::AssetLease<MaterialInstanceAsset> originalMaterial(m_materialRequest.GetAssetSlot());
            const IO::AssetLease<MaterialInstanceAsset> warmMaterial(m_warmMaterialRequest.GetAssetSlot());
            const IO::AssetLease<MaterialInstanceAsset> coolMaterial(m_coolMaterialRequest.GetAssetSlot());
            for (int32_t batchIndex = -1; batchIndex <= 1; ++batchIndex)
            {
                instanceDesc.m_material = batchIndex < 0 ? warmMaterial : batchIndex > 0 ? coolMaterial : originalMaterial;
                const float batchX = static_cast<float>(batchIndex) * 2.5f;
                MeshBatchDesc batchDesc;
                batchDesc.m_bounds = Aabb{ Vector3(batchX - 5.0f, -5.0f, -5.0f), Vector3(batchX + 5.0f, 5.0f, 5.0f) };
                batchDesc.m_drawTagMask = DrawTagMask(DrawTags::DepthPrepass) | DrawTagMask(DrawTags::Opaque);
                instanceDesc.m_batch = meshSceneModule.CreateBatch(batchDesc);

                for (int32_t instanceIndex = -1; instanceIndex <= 1; ++instanceIndex)
                {
                    instanceDesc.m_transform = Matrix4x4::RotationY(Constants::kPI)
                        * Matrix4x4::Translation(Vector3(batchX, 0.0f, static_cast<float>(instanceIndex) * 1.5f));
                    m_meshInstances.push_back(meshSceneModule.CreateInstance(instanceDesc));
                }
            }

            MeshBatchDesc culledBatchDesc;
            instanceDesc.m_material = originalMaterial;
            culledBatchDesc.m_bounds = Aabb{ Vector3(95.0f, -5.0f, -5.0f), Vector3(105.0f, 5.0f, 5.0f) };
            culledBatchDesc.m_drawTagMask = DrawTagMask(DrawTags::DepthPrepass) | DrawTagMask(DrawTags::Opaque);
            instanceDesc.m_batch = meshSceneModule.CreateBatch(culledBatchDesc);
            instanceDesc.m_transform = Matrix4x4::Translation(Vector3(100.0f, 0.0f, 0.0f));
            m_meshInstances.push_back(meshSceneModule.CreateInstance(instanceDesc));
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
        IO::AssetRequest m_warmMaterialRequest;
        IO::AssetRequest m_coolMaterialRequest;
        MaterialStreamer* m_materialStreamer = nullptr;
        MeshStreamer* m_meshStreamer = nullptr;
        TextureStreamer* m_textureStreamer = nullptr;
        festd::vector<MeshHandle> m_meshInstances;
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
