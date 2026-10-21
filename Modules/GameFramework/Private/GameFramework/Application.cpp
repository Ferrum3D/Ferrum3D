#include <Core/IO/Artifact.h>
#include <Core/IO/AssetManager.h>
#include <Core/Reflection.gen.h>
#include <Core/Threading/Thread.h>
#include <Core/Time/BaseTime.h>
#include <Framework/Entities/WorldTimeService.h>
#include <Framework/Input/WorldInputService.h>
#include <Framework/Reflection.gen.h>
#include <GameFramework/Application.h>
#include <GameFramework/Reflection.gen.h>
#include <GameFramework/Services/WorldGraphicsSceneService.h>
#include <GameFramework/Services/WorldPhysicsService.h>
#include <GameFramework/Services/WorldStreamingService.h>
#include <GameFramework/Systems/CameraSystem.h>
#include <GameFramework/Systems/MeshSystem.h>
#include <GameFramework/Systems/TransformationSystem.h>
#include <Graphics/Assets/MaterialAssets.h>
#include <Graphics/Assets/Streamers.h>
#include <Graphics/Core/Device.h>
#include <Graphics/Core/DeviceFactory.h>
#include <Graphics/Core/PipelineFactory.h>
#include <Graphics/Core/PipelineVariantSet.h>
#include <Graphics/Core/Reflection.gen.h>
#include <Graphics/Core/ResourcePool.h>
#include <Graphics/Core/Viewport.h>
#include <Graphics/Materials/MaterialInstance.h>
#include <Graphics/Reflection.gen.h>
#include <Graphics/Renderer.h>
#include <Graphics/Scene/View.h>

namespace FE::GameFramework
{
    using namespace Graphics;

    Application::Application()
    {
        // Retain the generated reflection needed to construct asset-selected systems and services.
        CallLinkerAnchor_Core();
        CallLinkerAnchor_Framework();
        CallLinkerAnchor_GraphicsCore();
        CallLinkerAnchor_Graphics();
        CallLinkerAnchor_GameFramework();
    }


    Application::~Application()
    {
        FE_Assert(!m_device, "Call Shutdown while the job system is running");
    }


    bool Application::Initialize(const ApplicationSettings& settings)
    {
        m_visibilityDebugMode = settings.m_visibilityDebugMode;
        FE_PROFILER_ZONE();
        FE_Assert(Threading::IsMainThread() && !m_device);

        InitializeWindow();
        m_mainWindow->Show(Framework::Core::PlatformWindowShowMode::kNormal);
        m_mainWindow->SetSize(settings.m_windowSize);

        Core::DeviceFactory::Init(settings.m_graphicsApi);
        auto& factory = Core::DeviceFactory::Get();
        const auto adapters = factory.EnumerateAdapters();
        FE_Assert(!adapters.empty());
        m_device = factory.CreateDevice(adapters.front().m_name);
        Renderer::Init(m_device.Get());
        auto& renderer = Renderer::Get();
        auto* pool = renderer.GetResourcePool();
        auto* graphicsQueue = renderer.GetGraphicsQueue();

        IO::ArtifactStore::SetCatalogSource(settings.m_assetDirectory);
        IO::AssetManager::Init();
        m_meshStreamer = Memory::DefaultNew<MeshStreamer>(m_device.Get(), pool, renderer.GetAsyncCopyQueue(), graphicsQueue);
        m_textureStreamer = Memory::DefaultNew<TextureStreamer>(m_device.Get(),
                                                                pool,
                                                                renderer.GetAsyncCopyQueue(),
                                                                graphicsQueue,
                                                                renderer.GetDescriptorManager());
        IO::AssetManager::RegisterStreamer(Rtti::GetTypeID<MeshAsset>(), m_meshStreamer);
        IO::AssetManager::RegisterStreamer(Rtti::GetTypeID<TextureAsset>(), m_textureStreamer);

        m_viewport = m_device->CreateViewport(pool, graphicsQueue);
        m_viewport->Init(Core::ViewportDesc::Create(m_mainWindow->GetClientRect(), m_mainWindow->GetNativeHandle().m_value));
        m_pipelineFactory = m_device->CreatePipelineFactory(renderer.GetDescriptorManager());

        m_materialStreamer =
            Memory::DefaultNew<MaterialStreamer>(renderer.GetMaterialParameterAllocator(), m_pipelineFactory.Get());
        IO::AssetManager::RegisterStreamer(Rtti::GetTypeID<MaterialInstanceAsset>(), m_materialStreamer);
        Core::CompileGlobalPipelineSets(m_pipelineFactory.Get());
        Core::WaitForGlobalPipelineSets();

        m_world.reset(Memory::DefaultNew<Framework::EntityWorldInstance>());
        m_previousUpdateTicks = Platform::GetTicks();
        if (!settings.m_world.IsValid())
        {
            // An empty application world still has its engine services for explicit system/entity registration.
            Framework::EntityWorldAsset definition;
            definition.m_services = { Rtti::GetTypeID<Framework::WorldTimeService>(),
                                      Rtti::GetTypeID<Input::WorldInputService>(),
                                      Rtti::GetTypeID<WorldGraphicsSceneService>(),
                                      Rtti::GetTypeID<WorldPhysicsService>(),
                                      Rtti::GetTypeID<WorldStreamingService>() };
            m_worldDefinitionLoaded = m_world->Load(definition);
            return m_worldDefinitionLoaded;
        }

        m_worldRequest = IO::AssetManager::LoadAsset(IO::Link<Framework::EntityWorldAsset>(settings.m_world));
        return m_worldRequest.IsValid();
    }


    bool Application::UpdateWorldLoading()
    {
        FE_PROFILER_ZONE();

        if (m_worldDefinitionLoaded || !m_worldRequest.IsCompleted())
            return true;

        if (m_worldRequest.GetResult() != IO::AssetLoadResult::kSucceeded)
        {
            Logger::LogError("World asset is unavailable: {}", m_worldRequest.GetError());
            return false;
        }

        const auto definition = IO::AssetHandle<Framework::EntityWorldAsset>(m_worldRequest.GetAssetSlot()).Read();
        if (!definition)
        {
            Logger::LogError("World asset is unavailable");
            return false;
        }

        if (!m_world->Load(*definition.Get()))
        {
            Logger::LogError("World asset is invalid");
            return false;
        }

        m_worldDefinitionLoaded = true;
        return true;
    }


    int32_t Application::Run(const uint32_t maxFrames)
    {
        FE_Assert(Threading::IsMainThread() && m_world);

        const auto joinRender = festd::defer([] {
            Renderer::Get().WaitForRender();
        });

        uint32_t frames = 0;
        while (!m_platformApplication->IsCloseRequested())
        {
            FE_PROFILER_ZONE_NAMED("Frame");

            m_platformApplication->PollEvents();
            if (m_platformApplication->IsCloseRequested())
                break;

            FrameMark;
            GetWorld().BeginFrame();

            // Streamers mutate live residency and material runtimes read during pass setup.
            Renderer::Get().WaitForScene();
            IO::AssetManager::Tick();
            if (!UpdateWorldLoading())
                return 1;

            if (m_worldDefinitionLoaded)
            {
                if (const Rc<WaitGroup> waitGroup = ScheduleUpdate())
                    waitGroup->Wait();
            }

            const auto status = m_world->GetStatus();
            const bool loadFailed = status.m_state == Framework::MaterializationState::kFailed
                || status.m_state == Framework::MaterializationState::kCanceled;
            if (loadFailed)
            {
                Logger::LogError("World asset is invalid: {}", status.m_error);
                return 1;
            }

            const bool worldReady = m_worldDefinitionLoaded && status.m_state == Framework::MaterializationState::kReady;
            if (worldReady)
                m_worldRequest.Reset();

            auto* graphics = GetWorld().FindService<WorldGraphicsSceneService>();
            Scene* scene = graphics ? &graphics->GetScene() : nullptr;
            if (scene != nullptr)
            {
                for (uint32_t viewIndex = 0; viewIndex < scene->GetViewCount(); ++viewIndex)
                    scene->GetView(viewIndex)->SetVisibilityDebugMode(m_visibilityDebugMode);
            }

            Renderer::Get().RenderAsync(scene, m_viewport.Get());

            if (maxFrames && worldReady && ++frames == maxFrames)
                break;
        }

        return 0;
    }


    void Application::Shutdown()
    {
        FE_PROFILER_ZONE();

        FE_Assert(Threading::IsMainThread());
        if (!m_device)
            return;

        Renderer::Get().WaitForRender();
        m_device->WaitIdle();
        m_world.reset();
        m_worldRequest.Reset();
        m_worldDefinitionLoaded = false;
        m_viewport.Reset();

        IO::AssetManager::Shutdown();
        Memory::DefaultDelete(m_materialStreamer);
        Memory::DefaultDelete(m_textureStreamer);
        Memory::DefaultDelete(m_meshStreamer);

        m_pipelineFactory.Reset();
        Renderer::Shutdown();
        m_device.Reset();
        Core::DeviceFactory::Shutdown();

        m_materialStreamer = nullptr;
        m_textureStreamer = nullptr;
        m_meshStreamer = nullptr;
    }


    Framework::EntityWorld& Application::GetWorld()
    {
        FE_Assert(m_world);
        return m_world->GetWorld();
    }


    void Application::ScheduleWorldUpdate(Framework::EntityWorld& world)
    {
        world.SchedulePhase(Phases::Input);
        world.SchedulePhase(Phases::Transformation);
        world.SchedulePhase(Phases::GraphicsExtraction);
    }


    void Application::ScheduleWorldFixedUpdate(Framework::EntityWorld& world)
    {
        world.SchedulePhase(Framework::Phases::PreUpdate);
        world.SchedulePhase(Framework::Phases::Update);
        world.SchedulePhase(Framework::Phases::PostUpdate);
    }


    Rc<WaitGroup> Application::ScheduleUpdate()
    {
        FE_PROFILER_ZONE();

        auto& world = GetWorld();
        const uint64_t ticks = Platform::GetTicks();
        const double elapsed = static_cast<double>(ticks - m_previousUpdateTicks) * Platform::GetSecondsPerTick();
        m_previousUpdateTicks = ticks;

        [[maybe_unused]] const bool fixedUpdated = world.RunFixedUpdates(elapsed, [this](Framework::EntityWorld& fixedWorld) {
            ScheduleWorldFixedUpdate(fixedWorld);
        });
        FE_AssertDebug(fixedUpdated);

        world.BeginUpdate();
        ScheduleWorldUpdate(world);

        [[maybe_unused]] const bool executed = world.ExecuteSchedule();
        FE_AssertDebug(executed);

        [[maybe_unused]] const bool ended = world.EndUpdate();
        FE_AssertDebug(ended);

        return nullptr;
    }


    void Application::DoRelease()
    {
        Memory::DefaultDelete(this);
    }
} // namespace FE::GameFramework
