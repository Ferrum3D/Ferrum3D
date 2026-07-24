#include <Core/Jobs/Jobs.h>
#include <Core/Math/Matrix4x4.h>
#include <Framework/Application/Application.h>
#include <Framework/Module.h>
#include <Graphics/Assets/IModelAssetManager.h>
#include <Graphics/Core/Device.h>
#include <Graphics/Core/DeviceFactory.h>
#include <Graphics/Core/PipelineFactory.h>
#include <Graphics/Core/PipelineVariantSet.h>
#include <Graphics/Core/Viewport.h>
#include <Graphics/Features/Mesh/MeshSceneModule.h>
#include <Graphics/Module.h>
#include <Graphics/Passes/DepthPrepass.h>
#include <Graphics/Passes/DrawTags.h>
#include <Graphics/Passes/OpaquePass.h>
#include <Graphics/Renderer.h>

using namespace FE;
using namespace FE::Graphics;

inline constexpr const char* kExampleName = "Ferrum3D - Renderer Sample";

namespace
{
    struct ExampleApplication final : public Framework::Application
    {
        ~ExampleApplication() override
        {
            Renderer::Shutdown();
        }

        void InitializeApp()
        {
            FE_PROFILER_ZONE();

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

            const RectInt clientRect = m_mainWindow->GetClientRect();
            m_viewport = m_device->CreateViewport(resourcePool, graphicsQueue);
            m_viewport->Init(Core::ViewportDesc::Create(clientRect, m_mainWindow->GetNativeHandle().m_value));

            m_pipelineFactory = m_device->CreatePipelineFactory();
            Core::CompileGlobalPipelineSets(m_pipelineFactory.Get());
            Core::WaitForGlobalPipelineSets();

            m_scene = renderer.CreateScene();
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

            IModelAssetManager* modelAssetManager = serviceProvider->ResolveRequired<IModelAssetManager>();
            m_model = modelAssetManager->Load("Models/StanfordBunny.fmd");
            m_model->m_completionWaitGroup->Wait();
            FE_Assert(m_model->m_status == AssetLoadingStatus::kCompletelyLoaded);

            auto& meshSceneModule = m_scene->GetModules().Find<MeshSceneModule>();

            MeshBatchDesc batchDesc;
            batchDesc.m_bounds = Aabb{ Vector3(-10.0f, -10.0f, -10.0f), Vector3(10.0f, 10.0f, 10.0f) };
            batchDesc.m_drawTagMask = DrawTagMask(DrawTags::DepthPrepass) | DrawTagMask(DrawTags::Opaque);
            m_batch = meshSceneModule.CreateBatch(batchDesc);

            MeshInstanceDesc instanceDesc;
            instanceDesc.m_asset = m_model.Get();
            instanceDesc.m_batch = m_batch;
            instanceDesc.m_transform = Matrix4x4::RotationX(Constants::kPI * 0.5f);
            instanceDesc.m_transform *= Matrix4x4::RotationY(Constants::kPI);

            m_mesh = meshSceneModule.CreateInstance(instanceDesc);
        }

        Rc<WaitGroup> ScheduleUpdate() override
        {
            FE_PROFILER_ZONE();
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

        Rc<ModelAsset> m_model;
        MeshBatch* m_batch = nullptr;
        MeshHandle m_mesh;
    };
} // namespace

int main(const int32_t argc, const char** argv)
{
    Env::ApplicationInfo applicationInfo;
    applicationInfo.m_name = kExampleName;

    Graphics::Module::Init();
    Framework::Module::Init();
    Env::Init(applicationInfo, argc, argv);

    std::pmr::memory_resource* allocator = Env::GetStaticAllocator(Memory::StaticAllocatorType::kLinear);
    auto* application = Memory::New<ExampleApplication>(allocator);
    application->InitializeCore();

    int32_t exitCode = 0;
    Jobs::DispatchMainThread([application, &exitCode] {
        application->InitializeWindow();
        application->InitializeApp();
        exitCode = application->Run();
    });

    Jobs::StartJobSystem();

    Memory::Delete(allocator, application);
    Env::Module::ShutdownModules();
    return exitCode;
}
