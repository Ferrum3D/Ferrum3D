#include <Graphics/Core/Device.h>
#include <Graphics/Features/ImGui/ImGuiRenderer.h>
#include <Graphics/Features/ImGui/ImGuiSceneModule.h>
#include <Graphics/Passes/ImGuiPass.h>
#include <imgui.h>

namespace FE::Graphics
{
    namespace
    {
        constexpr ImGuiBackendFlags kRendererBackendFlags = ImGuiBackendFlags_RendererHasVtxOffset
            | ImGuiBackendFlags_RendererHasTextures | ImGuiBackendFlags_RendererHasViewports;

        constexpr int kMaxTextureSize = 8192;
    } // namespace


    struct ImGuiRenderer::ViewportData final
    {
        Rc<Core::Viewport> m_target;
        Rc<View> m_view;
        bool m_ownsTarget = false;
    };


    ImGuiRenderer::ImGuiRenderer(Core::Device* device, Core::Viewport* mainViewport)
        : m_device(device)
    {
        FE_PROFILER_ZONE();

        ImGuiIO& io = ImGui::GetIO();
        FE_Assert(io.BackendRendererUserData == nullptr, "ImGui renderer backend is already initialized");
        io.BackendRendererUserData = this;
        io.BackendRendererName = "Ferrum3D";
        io.BackendFlags |= kRendererBackendFlags;

        ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();
        platformIO.Renderer_TextureMaxWidth = kMaxTextureSize;
        platformIO.Renderer_TextureMaxHeight = kMaxTextureSize;
        platformIO.Renderer_CreateWindow = &CreateViewportWindow;
        platformIO.Renderer_DestroyWindow = &DestroyViewportWindow;

        m_scene = Renderer::Get().CreateScene();
        m_scene->GetModules().Add<ImGuiSceneModule>();
        m_module = &m_scene->GetModules().Find<ImGuiSceneModule>();

        ImGuiViewport* main = ImGui::GetMainViewport();
        main->RendererUserData = CreateViewportData(*main, mainViewport);
    }


    ImGuiRenderer::~ImGuiRenderer()
    {
        FE_PROFILER_ZONE();

        Renderer& renderer = Renderer::Get();
        renderer.WaitForRender();

        // Destroys the secondary platform windows too, so it must run while the platform backend is alive.
        ImGui::DestroyPlatformWindows();
        m_module->ReleaseTextures();

        ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();
        platformIO.ClearRendererHandlers();
        platformIO.Renderer_TextureMaxWidth = 0;
        platformIO.Renderer_TextureMaxHeight = 0;

        ImGuiIO& io = ImGui::GetIO();
        io.BackendRendererUserData = nullptr;
        io.BackendRendererName = nullptr;
        io.BackendFlags &= ~kRendererBackendFlags;

        m_scene->GetModules().Remove<ImGuiSceneModule>();
        renderer.DestroyScene(m_scene.Get());
        m_scene.Reset();
    }


    void ImGuiRenderer::Submit()
    {
        FE_PROFILER_ZONE();

        const ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();
        m_module->SetFrame({ platformIO.Textures.Data, static_cast<uint32_t>(platformIO.Textures.Size) });

        for (const ImGuiViewport* viewport : platformIO.Viewports)
        {
            if (viewport->RendererUserData == nullptr || viewport->DrawData == nullptr)
                continue;

            if (viewport->DrawData->Valid && !(viewport->Flags & ImGuiViewportFlags_IsMinimized))
                m_module->SetDrawData(viewport->ID, viewport->DrawData);
        }
    }


    void ImGuiRenderer::AddViews(RenderRequest& request) const
    {
        for (const ImGuiViewport* viewport : ImGui::GetPlatformIO().Viewports)
        {
            if (m_module->FindDrawData(viewport->ID) == nullptr)
                continue;

            const auto* data = static_cast<const ViewportData*>(viewport->RendererUserData);
            request.AddView(data->m_view.Get(), data->m_target.Get());
        }
    }


    void ImGuiRenderer::CreateViewportWindow(ImGuiViewport* viewport)
    {
        FE_PROFILER_ZONE();

        const auto nativeWindow = reinterpret_cast<uintptr_t>(viewport->PlatformHandleRaw);
        FE_Assert(nativeWindow != 0, "The ImGui platform backend must provide native window handles");

        // Swapchain creation drains the graphics queue, which the render job must not use concurrently.
        Renderer& renderer = Renderer::Get();
        renderer.WaitForRender();

        ImGuiRenderer& self = Get();
        const Rc target = self.m_device->CreateViewport(renderer.GetResourcePool(), renderer.GetGraphicsQueue());
        target->Init(Core::ViewportDesc::Create(RectF(0.0f, 0.0f, viewport->Size.x, viewport->Size.y), nativeWindow));

        ViewportData* data = self.CreateViewportData(*viewport, target.Get());
        data->m_ownsTarget = true;
        viewport->RendererUserData = data;
    }


    void ImGuiRenderer::DestroyViewportWindow(ImGuiViewport* viewport)
    {
        FE_PROFILER_ZONE();

        auto* data = static_cast<ViewportData*>(viewport->RendererUserData);
        if (data == nullptr)
            return;

        // The previous frame retains views and swapchains; release them before the platform window is destroyed.
        Renderer::Get().WaitForRender();
        Get().m_scene->DestroyView(data->m_view.Get());
        if (data->m_ownsTarget)
            data->m_target->SetImmediateDestroyPolicy();

        Memory::DefaultDelete(data);
        viewport->RendererUserData = nullptr;
    }


    ImGuiRenderer& ImGuiRenderer::Get()
    {
        auto* renderer = static_cast<ImGuiRenderer*>(ImGui::GetIO().BackendRendererUserData);
        FE_Assert(renderer != nullptr);
        return *renderer;
    }


    ImGuiRenderer::ViewportData* ImGuiRenderer::CreateViewportData(const ImGuiViewport& viewport, Core::Viewport* target)
    {
        auto* data = Memory::DefaultNew<ViewportData>();
        data->m_target = target;
        data->m_view = m_scene->CreateView();
        data->m_view->GetModules().Add<ImGuiPass::ViewModule>();
        data->m_view->GetModules().Find<ImGuiPass::ViewModule>().m_viewportID = viewport.ID;
        return data;
    }
} // namespace FE::Graphics
