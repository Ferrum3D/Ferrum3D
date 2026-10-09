#include <Core/IO/Artifact.h>
#include <Core/IO/AssetManager.h>
#include <Core/Reflection.gen.h>
#include <Core/Threading/Thread.h>
#include <Framework/Reflection.gen.h>
#include <GameFramework/Application.h>
#include <GameFramework/GraphicsSystems.h>
#include <GameFramework/Reflection.gen.h>
#include <GameFramework/WorldStreamingService.h>
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

namespace FE::GameFramework
{
    using namespace Graphics;

    Application::Application()
    {
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
        if (!settings.m_world.IsValid())
        {
            // An empty application world still has its engine services for explicit system/entity registration.
            Framework::EntityWorldAsset definition;
            definition.m_services = { Rtti::GetTypeID<WorldGraphicsSceneService>(), Rtti::GetTypeID<WorldStreamingService>() };
            return m_world->Load(definition);
        }

        return LoadWorld(settings.m_world);
    }


    bool Application::LoadWorld(const IO::AssetID asset)
    {
        FE_PROFILER_ZONE();
        auto request = IO::AssetManager::LoadAsset(IO::Link<Framework::EntityWorldAsset>(asset));
        while (!request.IsCompleted())
        {
            IO::AssetManager::Tick();
            Threading::Sleep(1);
        }

        const auto definition = IO::AssetHandle<Framework::EntityWorldAsset>(request.GetAssetSlot()).Read();
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

        // Definition loading completes before per-entity dependency acquisitions and lifecycle activation.
        for (;;)
        {
            IO::AssetManager::Tick();
            if (!m_world->GetWorld().CommitBootstrap())
                return false;

            const auto status = m_world->GetStatus();
            if (status.m_state == Framework::MaterializationState::kReady)
                break;

            if (status.m_state != Framework::MaterializationState::kPending)
            {
                Logger::LogError("World asset is invalid: {}", status.m_error);
                return false;
            }

            Threading::Sleep(1);
        }

        return true;
    }


    int32_t Application::Run(const uint32_t maxFrames)
    {
        FE_Assert(Threading::IsMainThread() && m_world);
        uint32_t frames = 0;
        while (!m_platformApplication->IsCloseRequested())
        {
            FE_PROFILER_ZONE_NAMED("Frame");
            m_platformApplication->PollEvents();
            if (m_platformApplication->IsCloseRequested())
                break;

            FrameMark;
            if (const Rc<WaitGroup> waitGroup = ScheduleUpdate())
                waitGroup->Wait();

            if (auto* graphics = GetWorld().FindService<WorldGraphicsSceneService>())
                Renderer::Get().Render(&graphics->GetScene(), m_viewport.Get());

            if (maxFrames && ++frames == maxFrames)
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

        m_device->WaitIdle();
        m_world.reset();
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
        world.SchedulePhase(Phases::Transformation);
        world.SchedulePhase(Phases::GraphicsExtraction);
    }


    Rc<WaitGroup> Application::ScheduleUpdate()
    {
        FE_PROFILER_ZONE();
        IO::AssetManager::Tick();
        auto& world = GetWorld();
        world.BeginUpdate();
        ScheduleWorldUpdate(world);
        const bool executed = world.ExecuteSchedule();
        FE_Assert(executed);
        const bool ended = world.EndUpdate();
        FE_Assert(ended);
        return nullptr;
    }


    void Application::DoRelease()
    {
        Memory::DefaultDelete(this);
    }
} // namespace FE::GameFramework
