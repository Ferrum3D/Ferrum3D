#include <FeCore/Modules/Environment.h>
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
    RendererImpl::RendererImpl() {}


    RendererImpl::~RendererImpl() = default;


    Scene* RendererImpl::CreateScene()
    {
        EnsureDatabase();

        Rc<Scene> scene = Rc<SceneImpl>::New(std::pmr::get_default_resource(), this);
        m_scenes.push_back(scene);
        return scene.Get();
    }


    void RendererImpl::Render(Scene* scene, Core::Viewport* viewport)
    {
        FE_Assert(scene != nullptr);
        FE_Assert(viewport != nullptr);

        DI::IServiceProvider* serviceProvider = Env::GetServiceProvider();
        if (m_device == nullptr)
            m_device = serviceProvider->ResolveRequired<Core::Device>();

        EnsureDatabase();

        Core::GraphicsQueue* graphicsQueue = serviceProvider->ResolveRequired<Core::GraphicsQueue>();
        graphicsQueue->BeginFrame();
        viewport->AcquireNextImage();
        EnsureMainColorTarget(viewport->GetCurrentColorTarget()->GetDesc());
        EnsureMainDepthTarget(viewport->GetDesc());

        Rc<Core::FrameGraph> frameGraph = serviceProvider->ResolveRequired<Core::FrameGraph>();
        frameGraph->BeginFrame();

        m_database->Update(*frameGraph, {});

        for (uint32_t viewIndex = 0; viewIndex < scene->GetViewCount(); ++viewIndex)
        {
            frameGraph->GetBlackboard().Reset();
            View* view = scene->GetView(viewIndex);
            SetupFrameGraph(*frameGraph, frameGraph->GetBlackboard(), *scene, *view, *viewport);
        }

        viewport->PrepareBlit();
        Tools::Blit::AddPass(*frameGraph,
                             Core::TextureView::Create(m_mainColorTarget.Get()),
                             Core::TextureView::Create(viewport->GetCurrentColorTarget()));

        frameGraph->CompileAndExecute();
        viewport->Present();
        m_device->EndFrame();
    }


    void RendererImpl::EnsureDatabase()
    {
        if (m_database != nullptr)
            return;

        Core::ResourcePool* resourcePool = Env::GetServiceProvider()->ResolveRequired<Core::ResourcePool>();
        m_database = festd::make_unique<DB::Database>(resourcePool);
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

        DI::IServiceProvider* serviceProvider = Env::GetServiceProvider();
        Core::ResourcePool* resourcePool = serviceProvider->ResolveRequired<Core::ResourcePool>();

        m_mainColorTarget = resourcePool->CreateTexture("RendererMainColor", swapchainColorTargetDesc);
    }


    void RendererImpl::EnsureMainDepthTarget(const Core::ViewportDesc& viewportDesc)
    {
        const bool sizeMismatch = m_mainDepthTarget != nullptr
            && (m_mainDepthTarget->GetDesc().m_width != viewportDesc.m_width
                || m_mainDepthTarget->GetDesc().m_height != viewportDesc.m_height);
        if (m_mainDepthTarget != nullptr && !sizeMismatch)
            return;

        DI::IServiceProvider* serviceProvider = Env::GetServiceProvider();
        Core::ResourcePool* resourcePool = serviceProvider->ResolveRequired<Core::ResourcePool>();

        m_mainDepthTarget = resourcePool->CreateTexture("RendererMainDepth",
                                                        Core::Format::kD32_SFLOAT_S8_UINT,
                                                        { viewportDesc.m_width, viewportDesc.m_height });

        Core::ResourceCommitParams commitParams;
        commitParams.m_bindFlags = Core::BarrierAccessFlags::kDepthStencilRead | Core::BarrierAccessFlags::kDepthStencilWrite;
        commitParams.m_memory = Core::ResourceMemory::kDeviceLocal;
        resourcePool->CommitTextureMemory(m_mainDepthTarget.Get(), commitParams);
    }


    void RendererImpl::SetupFrameGraph(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard, Scene& scene, View& view,
                                       Core::Viewport& viewport)
    {
        Internal::RendererViewData& viewData = blackboard.Add<Internal::RendererViewData>();
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
} // namespace FE::Graphics
