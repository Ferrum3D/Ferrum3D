#include <Core/Env/Environment.h>
#include <Graphics/Core/AsyncCopyQueue.h>
#include <Graphics/Core/Device.h>
#include <Graphics/Core/FrameGraph/FrameGraph.h>
#include <Graphics/Core/GraphicsQueue.h>
#include <Graphics/Core/ResourcePool.h>
#include <Graphics/Database/Database.h>
#include <Graphics/Passes/DepthPrepass.h>
#include <Graphics/Passes/OpaquePass.h>
#include <Graphics/Passes/RendererPassCommon.h>
#include <Graphics/Passes/Tools/Blit.h>
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
    }


    RendererImpl::~RendererImpl()
    {
        m_device->WaitIdle();
    }


    Scene* RendererImpl::CreateScene()
    {
        EnsureDatabase();

        Rc<Scene> scene = Memory::DefaultNew<SceneImpl>(this);
        m_scenes.push_back(scene);
        return scene.Get();
    }


    void RendererImpl::Render(Scene* scene, Core::Viewport* viewport)
    {
        FE_Assert(scene != nullptr);
        FE_Assert(viewport != nullptr);

        EnsureDatabase();

        m_graphicsQueue->BeginFrame();
        viewport->AcquireNextImage();
        EnsureMainColorTarget(viewport->GetCurrentColorTarget()->GetDesc());
        EnsureMainDepthTarget(viewport->GetDesc());

        m_frameGraph->BeginFrame();

        m_database->Update(*m_frameGraph, m_graphicsQueue->GetCurrentFence());

        for (uint32_t viewIndex = 0; viewIndex < scene->GetViewCount(); ++viewIndex)
        {
            m_frameGraph->GetBlackboard().Reset();
            View* view = scene->GetView(viewIndex);
            SetupFrameGraph(*m_frameGraph, m_frameGraph->GetBlackboard(), *scene, *view, *viewport);
        }

        viewport->PrepareBlit();
        Tools::Blit::AddPass(*m_frameGraph,
                             Core::TextureView::Create(m_mainColorTarget.Get()),
                             Core::TextureView::Create(viewport->GetCurrentColorTarget()));

        m_frameGraph->CompileAndExecute();
        viewport->Present();
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


    void RendererImpl::EnsureDatabase()
    {
        if (m_database != nullptr)
            return;

        m_database = festd::make_unique<DB::Database>(m_device.Get(), m_resourcePool.Get());
    }


    void RendererImpl::EnsureMainColorTarget(const Core::TextureDesc& swapchainColorTargetDesc)
    {
        const bool sizeMismatch = m_mainColorTarget != nullptr
            && (m_mainColorTarget->GetDesc().m_width != swapchainColorTargetDesc.m_width
                || m_mainColorTarget->GetDesc().m_height != swapchainColorTargetDesc.m_height);
        const bool formatMismatch =
            m_mainColorTarget != nullptr && m_mainColorTarget->GetDesc().m_imageFormat != swapchainColorTargetDesc.m_imageFormat;
        if (m_mainColorTarget != nullptr && !sizeMismatch && !formatMismatch)
            return;

        m_mainColorTarget = Core::Texture::Create(m_device.Get(), "RendererMainColor", swapchainColorTargetDesc);
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
                                                  Core::Format::kD32_SFLOAT_S8_UINT,
                                                  { viewportDesc.m_width, viewportDesc.m_height });

        Core::ResourceCommitParams commitParams;
        commitParams.m_bindFlags = Core::BarrierAccessFlags::kDepthStencilRead | Core::BarrierAccessFlags::kDepthStencilWrite;
        commitParams.m_memory = Core::ResourceMemory::kDeviceLocal;
        m_resourcePool->CommitTextureMemory(m_mainDepthTarget.Get(), commitParams);
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

        view.Update(blackboard);

        DepthPrepass::AddPasses(graph, blackboard, scene);
        OpaquePass::AddPasses(graph, blackboard, scene);
    }


    void RendererImpl::DoRelease()
    {
        Memory::DefaultDelete(this);
    }
} // namespace FE::Graphics
