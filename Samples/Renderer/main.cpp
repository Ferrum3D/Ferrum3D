#include <FeCore/DI/Activator.h>
#include <FeCore/Math/Matrix4x4.h>
#include <FeCore/Modules/Configuration.h>
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
        ExampleApplication(const int32_t argc, const char** argv)
            : Application(argc, argv)
        {
        }

        ~ExampleApplication() override
        {
            if (m_device != nullptr)
                m_device->WaitIdle();
        }

        void InitializeApp()
        {
            FE_PROFILER_ZONE();

            DI::IServiceProvider* serviceProvider = Env::GetServiceProvider();

            m_logSink = festd::make_unique<Framework::StdoutLogSink>(serviceProvider->ResolveRequired<Logger>());

            m_factory = serviceProvider->ResolveRequired<Core::DeviceFactory>();
            for (const Core::AdapterInfo& adapterInfo : m_factory->EnumerateAdapters())
            {
                if (adapterInfo.m_kind == Core::AdapterKind::kDiscrete)
                {
                    m_factory->CreateDevice(adapterInfo.m_name);
                    break;
                }
            }

            m_device = serviceProvider->ResolveRequired<Core::Device>();

            const RectInt clientRect = m_mainWindow->GetClientRect();
            m_viewport = serviceProvider->ResolveRequired<Core::Viewport>();
            m_viewport->Init({ static_cast<uint32_t>(clientRect.Width()),
                               static_cast<uint32_t>(clientRect.Height()),
                               m_mainWindow->GetNativeHandle().m_value });

            Core::CompileGlobalPipelineSets(serviceProvider->ResolveRequired<Core::PipelineFactory>());
            Core::WaitForGlobalPipelineSets();

            m_renderer = serviceProvider->ResolveRequired<Renderer>();
            m_scene = m_renderer->CreateScene();
            m_scene->GetModules().Add<MeshSceneModule>();
            m_view = m_scene->CreateView();
            m_view->GetModules().Add<DepthPrepass::ViewModule>();
            m_view->GetModules().Add<OpaquePass::ViewModule>();

            const float aspectRatio = static_cast<float>(clientRect.Width()) / static_cast<float>(clientRect.Height());
            const Vector3 cameraPosition(0.0f, 2.5f, -2.0f);
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
            m_mesh = meshSceneModule.CreateInstance(instanceDesc);
        }

        Rc<WaitGroup> ScheduleUpdate() override
        {
            FE_PROFILER_ZONE();
            m_renderer->Render(m_scene, m_viewport.Get());
            return nullptr;
        }

        festd::unique_ptr<Framework::StdoutLogSink> m_logSink;

        Core::DeviceFactory* m_factory = nullptr;
        Core::Device* m_device = nullptr;
        Rc<Core::Viewport> m_viewport;

        Renderer* m_renderer = nullptr;
        Scene* m_scene = nullptr;
        View* m_view = nullptr;

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
    Env::Init(applicationInfo);

    std::pmr::memory_resource* allocator = Env::GetStaticAllocator(Memory::StaticAllocatorType::kLinear);

    auto* application = Memory::New<ExampleApplication>(allocator, argc, argv);
    application->InitializeCore();

    IJobSystem* jobSystem = Env::GetServiceProvider()->ResolveRequired<IJobSystem>();

    int32_t exitCode = 0;
    FunctorJob mainJob([application, &exitCode] {
        application->InitializeWindow();
        application->InitializeApp();
        exitCode = application->Run();
    });

    mainJob.Schedule(jobSystem, FiberAffinityMask::kMainThread);
    jobSystem->Start();

    Memory::Delete(allocator, application);
    Env::Module::ShutdownModules();
    return exitCode;
}
