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
#include <Graphics/Passes/GBufferDebugPass.h>
#include <Graphics/Passes/ImGuiPass.h>
#include <Graphics/Passes/RendererPassCommon.h>
#include <Graphics/Passes/SurfaceEvaluationPass.h>
#include <Graphics/Passes/TonemapPass.h>
#include <Graphics/Passes/VisibilityDebug.h>
#include <Graphics/Passes/VisibilityPass.h>
#include <Graphics/RendererImpl.h>
#include <Graphics/Scene/SceneImpl.h>
#include <Graphics/Scene/SurfaceEvaluation.h>

namespace FE::Graphics
{
    static Rc<RendererImpl> GRenderer;


    namespace
    {
        // Frame-local main targets are logical: the graph commits memory only for views whose passes use them.
        struct RendererViewTargets final
        {
            Rc<Core::Texture> m_color;
            Rc<Core::Texture> m_depth;
        };


        void ClearViewport(Core::FrameGraph& graph, Core::Viewport& viewport)
        {
            auto* clearPass = graph.AllocatePassData<RendererTargetClearPassDesc>();
            clearPass->m_colorTarget = Core::TextureView::Create(viewport.GetCurrentColorTarget());
            const Vector2UInt size = clearPass->m_colorTarget.m_target.GetBaseDesc().GetSize2D();
            clearPass->m_viewport = RectF::FromPosAndSize(Vector2::kZero, Vector2(size));
            graph.AddPass("ClearViewport", clearPass, [](Core::FrameGraphContext& context) {
                context.ClearColorTarget(0, Colors::kDarkSlateBlue);
                context.ClearRenderTargets();
            });
        }
    } // namespace


    void ClearMainTargets(Core::FrameGraph& graph, const RendererViewData& view)
    {
        auto* clearPass = graph.AllocatePassData<RendererTargetClearPassDesc>();
        clearPass->m_colorTarget = Core::TextureView::Create(view.m_mainColorTarget);
        clearPass->m_depthTarget = Core::TextureView::Create(view.m_mainDepthTarget);
        clearPass->m_viewport = view.m_viewportRect;
        graph.AddPass("ClearRendererTargets", clearPass, [](Core::FrameGraphContext& context) {
            context.ClearColorTarget(0, Colors::kDarkSlateBlue);
            context.ClearDepthStencilTarget(0.0f, 0);
            context.ClearRenderTargets();
        });
    }


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
        m_descriptorManager.Reset();
        m_device->WaitIdle();

        // Queued shader publications retain resources. Release them while the pool and transfer queue are still alive.
        m_graphicsQueue.Reset();
        m_device->WaitIdle();
        m_asyncCopyQueue.Reset();
        m_device->WaitIdle();
    }


    void RenderRequest::AddViewport(Core::Viewport* viewport)
    {
        FE_Assert(viewport != nullptr);

        const auto found = festd::find_if(m_viewports.begin(), m_viewports.end(), [viewport](const Rc<Core::Viewport>& entry) {
            return entry.Get() == viewport;
        });

        if (found == m_viewports.end())
            m_viewports.push_back(viewport);
    }


    void RenderRequest::AddView(View* view, Core::Viewport* viewport)
    {
        FE_Assert(view != nullptr);

        AddViewport(viewport);
        m_views.push_back({ view, viewport });
    }


    RenderRequest RenderRequest::CreateForScene(Scene* scene, Core::Viewport* viewport)
    {
        RenderRequest request;
        request.AddViewport(viewport);
        if (scene == nullptr)
            return request;

        for (uint32_t viewIndex = 0; viewIndex < scene->GetViewCount(); ++viewIndex)
            request.AddView(scene->GetView(viewIndex), viewport);

        return request;
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


    void RendererImpl::Render(RenderRequest request)
    {
        RenderAsync(std::move(request));
        WaitForRender();
    }


    void RendererImpl::RenderAsync(RenderRequest request)
    {
        FE_PROFILER_ZONE();
        FE_Assert(!request.m_viewports.empty());
        WaitForRender();
        m_request = std::move(request);
        m_sceneReady = WaitGroup::Create();
        m_renderComplete = WaitGroup::Create();
        Jobs::Dispatch(
            Jobs::FiberAffinityMask::kForegroundWorkers,
            festd::span<WaitGroup* const>{},
            Jobs::Priority::kNormal,
            [this, sceneReady = m_sceneReady] {
                RenderFrame(*sceneReady);
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

        // Release views and viewports on the joining thread, so callers may destroy their windows afterwards.
        m_request = {};
    }


    void RendererImpl::RenderFrame(WaitGroup& sceneReady)
    {
        FE_PROFILER_ZONE();

        FrameViewList views;
        PrepareFrameGraph(views);

        // Passes own copied upload data, resources and pipelines from this point onward.
        sceneReady.Signal();

        FrameViewportList viewports;
        AcquireViewports(viewports);
        AddOutputPasses(views, viewports);

        festd::inline_vector<Core::Viewport*, 4> presentedViewports;
        for (const FrameViewport& viewport : viewports)
        {
            if (viewport.m_isAcquired)
                presentedViewports.push_back(viewport.m_viewport);
        }

        m_frameGraph->CompileAndExecute();
        m_renderQueueUploader.CloseFrame(m_graphicsQueue->GetCurrentFence());
        m_graphicsQueue->SubmitFrame(presentedViewports);

        m_resourcePool->EndFrame();
        m_device->EndFrame();
    }


    void RendererImpl::PrepareFrameGraph(FrameViewList& views)
    {
        FE_PROFILER_ZONE();

        festd::inline_vector<Scene*, 4> scenes;
        for (const ViewRenderDesc& desc : m_request.m_views)
        {
            if (!desc.m_view->IsEnabled())
                continue;

            views.push_back({ desc.m_view.Get(), desc.m_viewport.Get(), nullptr });

            Scene* scene = desc.m_view->GetScene();
            if (festd::find(scenes.begin(), scenes.end(), scene) == scenes.end())
                scenes.push_back(scene);
        }

        m_graphicsQueue->BeginFrame();
        m_frameGraph->BeginFrame();

        for (Scene* scene : scenes)
        {
            scene->GetModules().ForEachActive([](SceneModuleBase& module) {
                module.Update();
            });
        }

        m_database->Update(*m_frameGraph, m_graphicsQueue->GetCurrentFence());
        m_materialParameters->Update(*m_frameGraph, m_graphicsQueue->GetCurrentFence());

        // Publish scene constants after database uploads establish the current table addresses.
        for (Scene* scene : scenes)
        {
            scene->GetModules().ForEachActive([&](SceneModuleBase& module) {
                module.UpdateRenderData(*m_frameGraph, m_renderQueueUploader);
            });
        }

        for (uint32_t viewIndex = 0; viewIndex < views.size(); ++viewIndex)
        {
            FE_FG_SCOPE(*m_frameGraph, Fmt::FixedFormat("View{}", viewIndex));
            views[viewIndex].m_blackboard = &m_frameGraph->CreateBlackboard();
            SetupView(views[viewIndex]);
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


    void RendererImpl::SetupView(const FrameView& frameView)
    {
        Core::FrameGraph& graph = *m_frameGraph;
        Core::FrameGraphBlackboard& blackboard = *frameView.m_blackboard;
        View& view = *frameView.m_view;
        Scene& scene = *view.GetScene();

        // Acquisition may resize the image later; output passes sample the main targets with normalized coordinates.
        const Core::ViewportDesc& viewportDesc = frameView.m_viewport->GetDesc();
        const Vector2UInt resolution(viewportDesc.m_width, viewportDesc.m_height);

        RendererViewTargets& targets = blackboard.Add<RendererViewTargets>();
        targets.m_color = Core::Texture::Create(m_device.Get(), "RendererMainColor", Core::Format::kB10G11R11_UFLOAT, resolution);
        targets.m_depth = Core::Texture::Create(m_device.Get(), "RendererMainDepth", Core::Format::kD32_SFLOAT, resolution);

        RendererViewData& viewData = blackboard.Add<RendererViewData>();
        viewData.m_scene = &scene;
        viewData.m_view = &view;
        viewData.m_mainColorTarget = targets.m_color.Get();
        viewData.m_mainDepthTarget = targets.m_depth.Get();
        viewData.m_viewportRect = viewportDesc.GetRect();
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
            module.PrepareRenderView(graph, blackboard, m_renderQueueUploader);
        });

        Visibility::AddPasses(graph, blackboard);
        Visibility::AddDebugPasses(graph, blackboard);
        SurfaceEvaluation::AddPasses(graph, blackboard);
        GBufferDebug::AddPasses(graph, blackboard);
        DeferredShading::AddPasses(graph, blackboard);
        Tonemap::AddPasses(graph, blackboard);
        ImGuiPass::AddPasses(graph, blackboard);
    }


    void RendererImpl::AcquireViewports(FrameViewportList& viewports)
    {
        FE_PROFILER_ZONE();

        for (const Rc<Core::Viewport>& viewport : m_request.m_viewports)
        {
            FrameViewport& frameViewport = viewports.emplace_back();
            frameViewport.m_viewport = viewport.Get();
            frameViewport.m_isAcquired = viewport->AcquireNextImage();
            if (frameViewport.m_isAcquired)
                viewport->PrepareBlit();
        }
    }


    void RendererImpl::AddOutputPasses(const FrameViewList& views, FrameViewportList& viewports)
    {
        FE_PROFILER_ZONE();

        for (uint32_t viewIndex = 0; viewIndex < views.size(); ++viewIndex)
        {
            const FrameView& view = views[viewIndex];
            const auto target = festd::find_if(viewports.begin(), viewports.end(), [&view](const FrameViewport& viewport) {
                return viewport.m_viewport == view.m_viewport;
            });

            FE_AssertDebug(target != viewports.end());
            if (!target->m_isAcquired)
                continue;

            FE_FG_SCOPE(*m_frameGraph, Fmt::FixedFormat("View{}Output", viewIndex));

            ViewOutput& output = view.m_blackboard->Add<ViewOutput>();
            output.m_target = Core::TextureView::Create(target->m_viewport->GetCurrentColorTarget());
            output.m_isInitialized = target->m_isInitialized;
            Tonemap::AddOutputPasses(*m_frameGraph, *view.m_blackboard);
            ImGuiPass::AddOutputPasses(*m_frameGraph, *view.m_blackboard);
            target->m_isInitialized = output.m_isInitialized;
        }

        for (const FrameViewport& viewport : viewports)
        {
            if (viewport.m_isAcquired && !viewport.m_isInitialized)
                ClearViewport(*m_frameGraph, *viewport.m_viewport);
        }
    }


    void RendererImpl::DoRelease()
    {
        Memory::DefaultDelete(this);
    }
} // namespace FE::Graphics
