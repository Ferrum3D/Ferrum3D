#include "ChildViewProbe.h"
#include <Graphics/Core/Device.h>
#include <Graphics/Core/DeviceFactory.h>
#include <Graphics/Core/PipelineFactory.h>
#include <Graphics/Core/PipelineVariantSet.h>
#include <Graphics/Features/Lighting/GlobalLightingSceneModule.h>
#include <Graphics/Features/Mesh/MeshSceneModule.h>
#include <Graphics/Features/Shadows/SunShadow.h>
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


TEST(Culling, ShadowViewsSelectLodsForTheirOwnResolution)
{
    // A clip level of 128 world units drawn into 4096 texels, with casters allowed two texels of error.
    Culling::PassData level;
    level.m_cameraPosition = Vector3(13.0f, 7.0f, -20.0f);
    level.m_viewProjection = Matrix4x4::Orthographic(128.0f, 128.0f, 0.0f, 1000.0f);
    level.m_projectionScaleY = 2.0f / 128.0f;
    level.m_isOrthographic = true;
    level.m_settings.m_lodErrorThreshold = 2.0f;

    // Orthographic views compare errors against a unit distance, so the scale is texels per world unit and threshold.
    const auto levelData = Culling::BuildMeshCullData(level, 4096.0f);
    EXPECT_FLOAT_EQ(levelData.m_cameraPosition.x, 13.0f);
    EXPECT_FLOAT_EQ(levelData.m_cameraPosition.w, 4096.0f / 128.0f / 2.0f);
    EXPECT_NE(levelData.m_flags & MeshPass::CullFlags::kOrthographic, 0u);
    EXPECT_NE(levelData.m_flags & MeshPass::CullFlags::kLodSelection, 0u);
    EXPECT_TRUE(Math::CmpEqual(levelData.m_viewProjection, level.m_viewProjection));

    // The mips of a local map project alike; each halves the resolution and with it the detail it selects.
    Culling::PassData map;
    map.m_projectionScaleY = 1.5f;
    map.m_settings.m_lodErrorThreshold = 2.0f;

    const auto mip0 = Culling::BuildMeshCullData(map, 4096.0f);
    const auto mip1 = Culling::BuildMeshCullData(map, 2048.0f);
    EXPECT_FLOAT_EQ(mip0.m_cameraPosition.w, 1.5f * 4096.0f * 0.5f / 2.0f);
    EXPECT_FLOAT_EQ(mip1.m_cameraPosition.w, mip0.m_cameraPosition.w * 0.5f);
    EXPECT_EQ(mip0.m_flags & MeshPass::CullFlags::kOrthographic, 0u);

    map.m_settings.m_lodSelection = false;
    EXPECT_EQ(Culling::BuildMeshCullData(map, 4096.0f).m_flags & MeshPass::CullFlags::kLodSelection, 0u);
}


TEST(ChildViews, ShadowViewsNeedNoChildViews)
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
    Rc<Core::PipelineFactory> pipelines = device->CreatePipelineFactory(renderer.GetDescriptorManager());
    Core::CompileGlobalPipelineSets(pipelines.Get());
    Core::WaitForGlobalPipelineSets();

    Scene* scene = renderer.CreateScene();
    scene->GetModules().Add<MeshSceneModule>();
    scene->GetModules().Add<GlobalLightingSceneModule>();
    GlobalLightingParameters lighting;
    lighting.m_directionalLightIlluminanceLux = 100000.0f;
    ASSERT_TRUE(scene->GetModules().Find<GlobalLightingSceneModule>().SetParameters(lighting));

    View* view = scene->CreateView();
    view->GetModules().Add<Culling::ViewModule>();
    view->GetModules().Add<Visibility::ViewModule>();
    view->GetModules().Add<SunShadow::ViewModule>();
    auto& shadows = view->GetModules().Find<SunShadow::ViewModule>();
    shadows.m_settings.m_levelCount = 2;
    view->SetProjection(Constants::kPI / 3.0f, 1.0f, 0.1f, 100.0f);

    Rc output = Core::Texture::Create(device.Get(), "ShadowChildViewOutput", Core::Format::kR8G8B8A8_UNORM, { 32, 32 });
    for (uint32_t frame = 0; frame < 2; ++frame)
    {
        RenderRequest request;
        request.AddView(view, output.Get());
        renderer.Render(std::move(request));
    }

    EXPECT_EQ(scene->GetViewCount(), 1u);

    output.Reset();
    scene->DestroyView(view);
    scene->GetModules().Remove<GlobalLightingSceneModule>();
    scene->GetModules().Remove<MeshSceneModule>();
    renderer.DestroyScene(scene);
    Renderer::Shutdown();
    pipelines.Reset();
    device->WaitIdle();
}
