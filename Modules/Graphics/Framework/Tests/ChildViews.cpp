#include "ChildViewProbe.h"
#include <Graphics/Core/Device.h>
#include <Graphics/Core/DeviceFactory.h>
#include <Graphics/Core/PipelineFactory.h>
#include <Graphics/Core/PipelineVariantSet.h>
#include <Graphics/Features/Mesh/MeshSceneModule.h>
#include <Graphics/Passes/RendererPassCommon.h>
#include <Graphics/Passes/VisibilityPass.h>
#include <Graphics/Renderer.h>
#include <Graphics/Scene/Culling.h>
#include <Graphics/Scene/Scene.h>
#include <Graphics/Scene/View.h>
#include <gtest/gtest.h>

using namespace FE;
using namespace FE::Graphics;

namespace FE::Graphics::ChildViewProbe
{
    ViewModule::ViewModule(View* view)
        : ViewModuleBase(view)
    {
    }


    void ViewModule::Update(Core::FrameGraphBlackboard&) {}


    void ViewModule::AddRenderPasses(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard)
    {
        Core::FrameGraphBlackboard& child = Renderer::Get().AddViewPasses(graph, { m_child.Get(), m_childResolution });
        if (const Visibility::Output* visibility = child.TryGet<Visibility::Output>())
            m_childVisibilitySize = visibility->m_target->GetDesc().GetSize2D();

        m_childBlackboardView = child.Get<RendererViewData>().m_view;
        m_isChildOrthographic = Culling::GetPassData(child).m_isOrthographic;
        m_isParentBlackboardSeparate = blackboard.TryGet<Visibility::Output>() == nullptr;
    }


    void ViewModule::DoRelease()
    {
        Memory::DefaultDelete(this);
    }
} // namespace FE::Graphics::ChildViewProbe


TEST(ChildViews, ParentPassSetupRendersChildView)
{
    Core::DeviceFactory::Init(Core::GraphicsApi::kVulkan);
    const auto shutdown = festd::defer([] {
        Core::DeviceFactory::Shutdown();
    });

    const auto adapters = Core::DeviceFactory::Get().EnumerateAdapters();
    ASSERT_FALSE(adapters.empty());
    const Rc device = Core::DeviceFactory::Get().CreateDevice(adapters.front().m_name);
    Renderer::Init(device.Get());
    Renderer& renderer = Renderer::Get();

    // The child's late culling phase builds a depth pyramid with global pipeline sets.
    Rc<Core::PipelineFactory> pipelines = device->CreatePipelineFactory(renderer.GetDescriptorManager());
    Core::CompileGlobalPipelineSets(pipelines.Get());
    Core::WaitForGlobalPipelineSets();

    Scene* scene = renderer.CreateScene();
    scene->GetModules().Add<MeshSceneModule>();

    // The child sees the scene from above through an orthographic projection, as a directional light would.
    Rc<View> child = scene->CreateChildView();
    child->GetModules().Add<Culling::ViewModule>();
    child->GetModules().Add<Visibility::ViewModule>();
    child->SetCameraTransform(Transform::Create(Vector3(0, 50, 0), Quaternion::RotationX(Constants::kPI * 0.5f), 1));
    child->SetOrthographicProjection(40, 40, 1, 100);
    EXPECT_EQ(scene->GetViewCount(), 0u);

    View* parent = scene->CreateView();
    parent->GetModules().Add<ChildViewProbe::ViewModule>();
    auto& probe = parent->GetModules().Find<ChildViewProbe::ViewModule>();
    probe.m_child = child;
    probe.m_childResolution = Vector2UInt(96, 64);

    Rc output = Core::Texture::Create(device.Get(), "ChildViewParentOutput", Core::Format::kR8G8B8A8_UNORM, { 32, 32 });
    RenderRequest request;
    request.AddView(parent, output.Get());
    renderer.Render(std::move(request));

    EXPECT_EQ(probe.m_childVisibilitySize, probe.m_childResolution);
    EXPECT_EQ(probe.m_childBlackboardView, child.Get());
    EXPECT_TRUE(probe.m_isChildOrthographic);
    EXPECT_TRUE(probe.m_isParentBlackboardSeparate);

    output.Reset();
    probe.m_child.Reset();
    child.Reset();
    scene->DestroyView(parent);
    scene->GetModules().Remove<MeshSceneModule>();
    renderer.DestroyScene(scene);
    Renderer::Shutdown();
    pipelines.Reset();
    device->WaitIdle();
}
