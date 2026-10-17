#include <Core/Env/Environment.h>
#include <Core/Jobs/Jobs.h>
#include <Core/Math/Colors.h>
#include <Graphics/Core/AsyncCopyQueue.h>
#include <Graphics/Core/Device.h>
#include <Graphics/Core/FrameGraph/FrameGraph.h>
#include <Graphics/Core/GraphicsQueue.h>
#include <Graphics/Core/ResourcePool.h>
#include <Graphics/Database/Database.h>
#include <Graphics/Passes/DeferredShadingPass.h>
#include <Graphics/Passes/RendererPassCommon.h>
#include <Graphics/Passes/SurfaceEvaluationPass.h>
#include <Graphics/Passes/Tools/Tonemap.h>
#include <Graphics/Passes/VisibilityPass.h>
#include <Graphics/RendererImpl.h>
#include <Graphics/Scene/SceneImpl.h>
#include <Graphics/Scene/SurfaceEvaluation.h>

namespace FE::Graphics
{
    static Rc<RendererImpl> GRenderer;


    void Renderer::Init(Core::Device* device)
    {
        GRenderer = Memory::DefaultNew<RendererImpl>(device);
    }


    void Renderer::Shutdown()
    {
        if (GRenderer)
            GRenderer->WaitForRender();

        GRenderer.Reset();
    }


    Renderer& Renderer::Get()
    {
        return *GRenderer;
    }


    RendererImpl::RendererImpl(Core::Device* device)
        : m_device(device)
    {
        m_graphicsQueue = m_device->CreateGraphicsQueue();
        m_asyncCopyQueue = m_device->CreateAsyncCopyQueue();

        m_descriptorManager = m_device->CreateDescriptorManager();
        m_resourcePool = m_device->CreateResourcePool(m_graphicsQueue.Get(), m_asyncCopyQueue.Get());
        m_frameGraph = m_device->CreateFrameGraph(m_descriptorManager.Get(), m_resourcePool.Get(), m_graphicsQueue.Get());
        m_renderQueueUploader.Setup("RenderQueueUploader", m_resourcePool.Get(), 16 * 1024 * 1024);

        m_database = festd::make_unique<DB::Database>(m_device.Get(), m_resourcePool.Get());
        m_materialParameters = festd::make_unique<MaterialParameterAllocator>(m_device.Get(), m_resourcePool.Get());
    }


    RendererImpl::~RendererImpl()
    {
        FE_PROFILER_ZONE();
        WaitForRender();
        m_asyncCopyQueue->Drain();
        m_device->WaitIdle();
        m_scenes.clear();
        m_renderQueueUploader.Shutdown();
        m_frameGraph.Reset();
        m_materialParameters.reset();
        m_database.reset();
        m_mainColorTarget.Reset();
        m_mainDepthTarget.Reset();
        m_descriptorManager.Reset();
        m_device->WaitIdle();

        // Queued shader publications retain resources. Release them while the pool and transfer queue are still alive.
        m_graphicsQueue.Reset();
        m_device->WaitIdle();
        m_asyncCopyQueue.Reset();
        m_device->WaitIdle();
    }


    Scene* RendererImpl::CreateScene()
    {
        WaitForScene();

        Rc<Scene> scene = Memory::DefaultNew<SceneImpl>(this);
        m_scenes.push_back(scene);
        return scene.Get();
    }


    void RendererImpl::DestroyScene(Scene* scene)
    {
        WaitForScene();

        const auto found = festd::find_if(m_scenes.begin(), m_scenes.end(), [scene](const Rc<Scene>& candidate) {
            return candidate.Get() == scene;
        });
        FE_Assert(found != m_scenes.end());
        m_scenes.erase(found);
    }


    void RendererImpl::Render(Scene* scene, Core::Viewport* viewport)
    {
        RenderAsync(scene, viewport);
        WaitForRender();
    }


    void RendererImpl::RenderAsync(Scene* scene, Core::Viewport* viewport)
    {
        FE_PROFILER_ZONE();
        FE_Assert(viewport != nullptr);
        WaitForRender();
        m_sceneReady = WaitGroup::Create();
        m_renderComplete = WaitGroup::Create();
        Jobs::Dispatch(
            Jobs::FiberAffinityMask::kForegroundWorkers,
            festd::span<WaitGroup* const>{},
            Jobs::Priority::kNormal,
            [this, scene = Rc<Scene>(scene), viewport = Rc<Core::Viewport>(viewport), sceneReady = m_sceneReady] {
                RenderFrame(scene.Get(), viewport.Get(), *sceneReady);
            },
            m_renderComplete.Get());
    }


    void RendererImpl::WaitForScene()
    {
        FE_PROFILER_ZONE();
        if (m_sceneReady)
            m_sceneReady->Wait();
    }


    void RendererImpl::WaitForRender()
    {
        FE_PROFILER_ZONE();
        if (m_renderComplete)
            m_renderComplete->Wait();
    }


    void RendererImpl::RenderFrame(Scene* scene, Core::Viewport* viewport, WaitGroup& sceneReady)
    {
        FE_PROFILER_ZONE();

        PrepareFrameGraph(scene, viewport);

        // Passes own copied upload data, resources and pipelines from this point onward.
        sceneReady.Signal();

        const bool acquired = viewport->AcquireNextImage();
        if (acquired)
        {
            viewport->PrepareBlit();
            Tools::Tonemap::AddPass(*m_frameGraph, m_mainColorTarget.Get(), viewport->GetCurrentColorTarget());
        }

        m_frameGraph->CompileAndExecute();
        m_renderQueueUploader.CloseFrame(m_graphicsQueue->GetCurrentFence());

        if (acquired)
            viewport->Present();
        else
            m_graphicsQueue->SubmitFrame();

        m_resourcePool->EndFrame();
        m_device->EndFrame();
    }


    void RendererImpl::PrepareFrameGraph(Scene* scene, Core::Viewport* viewport)
    {
        FE_PROFILER_ZONE();

        bool hasEnabledView = false;
        if (scene)
        {
            for (uint32_t i = 0; i < scene->GetViewCount(); ++i)
                hasEnabledView |= scene->GetView(i)->IsEnabled();
        }

        m_graphicsQueue->BeginFrame();

        // Logical targets are frame-local; the graph infers usage and obtains their memory from its pool.
        const Vector2UInt resolution(viewport->GetDesc().m_width, viewport->GetDesc().m_height);
        m_mainColorTarget =
            Core::Texture::Create(m_device.Get(), "RendererMainColor", Core::Format::kB10G11R11_UFLOAT, resolution);
        m_mainDepthTarget = Core::Texture::Create(m_device.Get(), "RendererMainDepth", Core::Format::kD32_SFLOAT, resolution);

        m_frameGraph->BeginFrame();

        if (hasEnabledView)
        {
            scene->GetModules().ForEachActive([](SceneModuleBase& module) {
                module.Update();
            });
        }

        m_database->Update(*m_frameGraph, m_graphicsQueue->GetCurrentFence());
        m_materialParameters->Update(*m_frameGraph, m_graphicsQueue->GetCurrentFence());

        if (hasEnabledView)
        {
            // Publish scene constants after database uploads establish the current table addresses.
            scene->GetModules().ForEachActive([&](SceneModuleBase& module) {
                module.UpdateRenderData(*m_frameGraph, m_renderQueueUploader);
            });

            for (uint32_t viewIndex = 0; viewIndex < scene->GetViewCount(); ++viewIndex)
            {
                View* view = scene->GetView(viewIndex);
                if (!view->IsEnabled())
                    continue;

                FE_FG_SCOPE(*m_frameGraph, Fmt::FixedFormat("View{}", viewIndex));
                m_frameGraph->GetBlackboard().Reset();
                SetupFrameGraph(*m_frameGraph, m_frameGraph->GetBlackboard(), *scene, *view, *viewport);
            }
        }
        else
        {
            ClearTargets(*m_frameGraph, *viewport);
        }
    }


    Core::GraphicsQueue* RendererImpl::GetGraphicsQueue() const
    {
        return m_graphicsQueue.Get();
    }


    Core::AsyncCopyQueue* RendererImpl::GetAsyncCopyQueue() const
    {
        return m_asyncCopyQueue.Get();
    }


    Core::ResourcePool* RendererImpl::GetResourcePool() const
    {
        return m_resourcePool.Get();
    }


    Core::DescriptorManager* RendererImpl::GetDescriptorManager() const
    {
        return m_descriptorManager.Get();
    }


    MaterialParameterAllocator* RendererImpl::GetMaterialParameterAllocator() const
    {
        return m_materialParameters.get();
    }


    void RendererImpl::ClearTargets(Core::FrameGraph& graph, Core::Viewport& viewport)
    {
        auto* clearPass = graph.AllocatePassData<RendererTargetClearPassDesc>();
        clearPass->m_colorTarget = Core::TextureView::Create(m_mainColorTarget.Get());
        clearPass->m_depthTarget = Core::TextureView::Create(m_mainDepthTarget.Get());
        clearPass->m_viewport = viewport.GetDesc().GetRect();
        graph.AddPass("ClearRendererTargets", clearPass, [](Core::FrameGraphContext& context) {
            context.ClearColorTarget(0, Colors::kDarkSlateBlue);
            context.ClearDepthStencilTarget(0.0f, 0);
            context.ClearRenderTargets();
        });
    }


    void RendererImpl::SetupFrameGraph(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard, Scene& scene, View& view,
                                       Core::Viewport& viewport)
    {
        RendererViewData& viewData = blackboard.Add<RendererViewData>();
        viewData.m_scene = &scene;
        viewData.m_view = &view;
        viewData.m_mainColorTarget = m_mainColorTarget.Get();
        viewData.m_mainDepthTarget = m_mainDepthTarget.Get();
        viewData.m_viewportRect = viewport.GetDesc().GetRect();
        viewData.m_renderQueueUploader = &m_renderQueueUploader;

        ViewData& constants = blackboard.Add<ViewData>();
        constants.m_viewProjection = view.GetViewProjectionMatrix();
        constants.m_inverseViewProjection = view.GetInverseViewProjectionMatrix();
        constants.m_cameraPosition = PackedVector4F(Vector4(view.GetCameraTransform().Translation(), 1.0f));

        const RectF rect = viewData.m_viewportRect;
        constants.m_viewport = PackedVector4F(rect.left, rect.top, rect.Width(), rect.Height());
        constants.m_backgroundColor = PackedVector4F(Vector4(Colors::kDarkSlateBlue));
        viewData.m_constants = Core::Buffer::CreateStructured<ViewData>(graph.GetDevice(), "View", 1);
        FE_Verify(m_renderQueueUploader.Upload(graph, viewData.m_constants.Get(), constants));

        blackboard.Add<SceneData*>(&scene.GetRenderData());
        blackboard.Add<SurfaceEvaluationInputs>();

        view.Update(blackboard);

        scene.GetModules().ForEachActive([&](SceneModuleBase& module) {
            module.PrepareRenderView(graph, m_renderQueueUploader);
        });

        const bool hasVisibility = blackboard.Contains<Visibility::PassData>();
        const bool hasSurface = blackboard.Contains<SurfaceEvaluation::ViewModule*>();
        const bool hasShading = blackboard.Contains<DeferredShading::ViewModule*>();
        const auto mode = view.GetVisibilityDebugMode();
        const bool needsSurface = Visibility::NeedsSurfaceEvaluation(mode);
        const bool needsShading = Visibility::NeedsDeferredShading(mode);
        if (!hasVisibility || (needsSurface && !hasSurface) || (needsShading && !hasShading))
            ClearTargets(graph, viewport);

        Visibility::AddPasses(graph, blackboard);
        SurfaceEvaluation::AddPasses(graph, blackboard);
        DeferredShading::AddPasses(graph, blackboard);
    }


    void RendererImpl::DoRelease()
    {
        Memory::DefaultDelete(this);
    }
} // namespace FE::Graphics
