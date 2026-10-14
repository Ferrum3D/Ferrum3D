#include <Core/Env/Environment.h>
#include <Core/Math/Colors.h>
#include <Graphics/Core/AsyncCopyQueue.h>
#include <Graphics/Core/Device.h>
#include <Graphics/Core/FrameGraph/FrameGraph.h>
#include <Graphics/Core/GraphicsQueue.h>
#include <Graphics/Core/ResourcePool.h>
#include <Graphics/Database/Database.h>
#include <Graphics/Passes/DepthPrepass.h>
#include <Graphics/Passes/OpaquePass.h>
#include <Graphics/Passes/RendererPassCommon.h>
#include <Graphics/Passes/Tools/Tonemap.h>
#include <Graphics/Passes/VisibilityPass.h>
#include <Graphics/Passes/VisibilitySupport.h>
#include <Graphics/RendererImpl.h>
#include <Graphics/Scene/SceneImpl.h>

namespace FE::Graphics
{
    static Rc<RendererImpl> GRenderer;


    void Renderer::Init(Core::Device* device)
    {
        GRenderer = Memory::DefaultNew<RendererImpl>(device);
    }


    void Renderer::Shutdown()
    {
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
        m_asyncCopyQueue->Drain();
        m_device->WaitIdle();
        m_scenes.clear();
        m_renderQueueUploader.Shutdown();
        m_frameGraph.Reset();
        m_materialParameters.reset();
        m_database.reset();
        m_mainColorTarget.Reset();
        m_mainDepthTarget.Reset();
        m_visibilityTarget.Reset();
        m_surfaceWork = {};
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
        Rc<Scene> scene = Memory::DefaultNew<SceneImpl>(this);
        m_scenes.push_back(scene);
        return scene.Get();
    }


    void RendererImpl::DestroyScene(Scene* scene)
    {
        const auto found = festd::find_if(m_scenes.begin(), m_scenes.end(), [scene](const Rc<Scene>& candidate) {
            return candidate.Get() == scene;
        });
        FE_Assert(found != m_scenes.end());
        m_scenes.erase(found);
    }


    void RendererImpl::Render(Scene* scene, Core::Viewport* viewport)
    {
        bool hasEnabledView = false;
        if (scene)
        {
            for (uint32_t i = 0; i < scene->GetViewCount(); ++i)
                hasEnabledView |= scene->GetView(i)->IsEnabled();
        }

        FE_Assert(viewport != nullptr);

        m_graphicsQueue->BeginFrame();
        viewport->AcquireNextImage();
        EnsureMainColorTarget(viewport->GetDesc());
        EnsureMainDepthTarget(viewport->GetDesc());

        m_frameGraph->BeginFrame();

        if (hasEnabledView)
        {
            scene->GetModules().ForEachActive([](SceneModuleBase& module) {
                module.Update();
            });

            scene->GetModules().ForEachActive([&](SceneModuleBase& module) {
                module.UpdateRenderData(*m_frameGraph, m_renderQueueUploader);
            });
        }

        m_database->Update(*m_frameGraph, m_graphicsQueue->GetCurrentFence());
        m_materialParameters->Update(*m_frameGraph, m_graphicsQueue->GetCurrentFence());

        if (hasEnabledView)
        {
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

        viewport->PrepareBlit();
        Tools::Tonemap::AddPass(*m_frameGraph,
                                Core::TextureView::Create(m_mainColorTarget.Get()),
                                Core::TextureView::Create(viewport->GetCurrentColorTarget()));

        m_frameGraph->CompileAndExecute();
        m_renderQueueUploader.CloseFrame(m_graphicsQueue->GetCurrentFence());
        viewport->Present();
        m_resourcePool->EndFrame();
        m_device->EndFrame();
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


    void RendererImpl::EnsureMainColorTarget(const Core::ViewportDesc& viewportDesc)
    {
        const bool sizeMismatch = m_mainColorTarget != nullptr
            && (m_mainColorTarget->GetDesc().m_width != viewportDesc.m_width
                || m_mainColorTarget->GetDesc().m_height != viewportDesc.m_height);
        if (m_mainColorTarget != nullptr && !sizeMismatch)
            return;

        m_mainColorTarget = Core::Texture::Create(m_device.Get(),
                                                  "RendererMainColor",
                                                  Core::Format::kB10G11R11_UFLOAT,
                                                  { viewportDesc.m_width, viewportDesc.m_height });

        Core::ResourceCommitParams commitParams;
        commitParams.m_bindFlags = Core::BarrierAccessFlags::kRenderTarget | Core::BarrierAccessFlags::kShaderRead
            | Core::BarrierAccessFlags::kShaderWrite;
        commitParams.m_memory = Core::ResourceMemory::kDeviceLocal;
        m_resourcePool->CommitTextureMemory(m_mainColorTarget.Get(), commitParams);
    }


    void RendererImpl::EnsureMainDepthTarget(const Core::ViewportDesc& viewportDesc)
    {
        const bool sizeMismatch = m_mainDepthTarget != nullptr
            && (m_mainDepthTarget->GetDesc().m_width != viewportDesc.m_width
                || m_mainDepthTarget->GetDesc().m_height != viewportDesc.m_height);
        if (m_mainDepthTarget != nullptr && !sizeMismatch)
            return;

        m_mainDepthTarget = Core::Texture::Create(m_device.Get(),
                                                  "RendererMainDepth",
                                                  Core::Format::kD32_SFLOAT,
                                                  { viewportDesc.m_width, viewportDesc.m_height });

        Core::ResourceCommitParams commitParams;
        commitParams.m_bindFlags = Core::BarrierAccessFlags::kDepthStencilRead | Core::BarrierAccessFlags::kDepthStencilWrite
            | Core::BarrierAccessFlags::kShaderRead;
        commitParams.m_memory = Core::ResourceMemory::kDeviceLocal;
        m_resourcePool->CommitTextureMemory(m_mainDepthTarget.Get(), commitParams);
    }


    void RendererImpl::EnsureVisibilityTarget(const Core::ViewportDesc& viewportDesc)
    {
        if (m_visibilityTarget != nullptr && m_visibilityTarget->GetDesc().m_width == viewportDesc.m_width
            && m_visibilityTarget->GetDesc().m_height == viewportDesc.m_height)
        {
            return;
        }

        const VisibilitySupport support = VisibilitySupport::Query(*m_device);
        FE_Assert(support.m_supported && support.m_hdrFormat == Core::Format::kB10G11R11_UFLOAT,
                  "Visibility debug rendering requires sampled D32, R32G32_UINT attachment and packed HDR storage writes");
        m_visibilityTarget = Core::Texture::Create(m_device.Get(),
                                                   "RendererVisibility",
                                                   Core::Format::kR32G32_UINT,
                                                   { viewportDesc.m_width, viewportDesc.m_height });
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
        viewData.m_viewport = &viewport;
        viewData.m_mainColorTarget = m_mainColorTarget.Get();
        viewData.m_mainDepthTarget = m_mainDepthTarget.Get();
        viewData.m_viewportRect = viewport.GetDesc().GetRect();
        viewData.m_database = m_database.get();
        viewData.m_renderQueueUploader = &m_renderQueueUploader;
        viewData.m_surfaceWork = &m_surfaceWork;

        view.Update(blackboard);

        scene.GetModules().ForEachActive([&](SceneModuleBase& module) {
            module.PrepareRenderView(graph, m_renderQueueUploader);
        });

        if (view.GetVisibilityDebugMode() != Visibility::DebugMode::kForward)
        {
            EnsureVisibilityTarget(viewport.GetDesc());
            Visibility::AddPasses(graph, blackboard, m_visibilityTarget.Get());
        }
        else
        {
            ClearTargets(graph, viewport);
            DepthPrepass::AddPasses(graph, blackboard);
            OpaquePass::AddPasses(graph, blackboard);
        }
    }


    void RendererImpl::DoRelease()
    {
        Memory::DefaultDelete(this);
    }
} // namespace FE::Graphics
