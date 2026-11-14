#include "Fsr3Probe.h"
#include <Graphics/Core/DeviceFactory.h>
#include <Graphics/Core/GraphicsQueue.h>
#include <Graphics/Core/PipelineFactory.h>
#include <Graphics/Core/PipelineVariantSet.h>
#include <Graphics/Core/ResourcePool.h>
#include <Graphics/Passes/ForwardTransparencyPass.h>
#include <Graphics/Passes/Fsr3UpscalerPass.h>
#include <Graphics/Passes/RendererPassCommon.h>
#include <Graphics/Passes/SurfaceEvaluationPass.h>
#include <Graphics/Passes/VisibilityPass.h>
#include <Graphics/Renderer.h>
#include <cmath>
#include <gtest/gtest.h>

using namespace FE;
using namespace FE::Graphics;

TEST(Fsr3Upscaler, PreservesHdrAcrossHistoryResetResizeAndNativeAA)
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
    auto* queue = renderer.GetGraphicsQueue();
    auto* pool = renderer.GetResourcePool();

    Rc pipelines = device->CreatePipelineFactory(renderer.GetDescriptorManager());
    Core::CompileGlobalPipelineSets(pipelines.Get());
    Core::WaitForGlobalPipelineSets();

    const auto pipeline = [&](const Env::Name path) {
        Core::ComputePipelineRequest request;
        request.m_desc.SetComputeShader(path);
        Core::ComputePipeline* result = pipelines->CreateComputePipeline(request);
        result->GetCompletionWaitGroup()->Wait();
        return result;
    };

    Core::ComputePipeline* fillPipeline = pipeline("Shaders/Tests/Fsr3Fill.cs.hlsl");
    Core::ComputePipeline* inspectPipeline = pipeline("Shaders/Tests/Fsr3Inspect.cs.hlsl");
    ASSERT_TRUE(fillPipeline->IsReady());
    ASSERT_TRUE(inspectPipeline->IsReady());

    Rc graph = device->CreateFrameGraph(renderer.GetDescriptorManager(), pool, queue);
    Scene* scene = renderer.CreateScene();
    View* view = scene->CreateView();
    view->SetProjection(1.0f, 129.0f / 73.0f, 0.1f, 1000.0f);
    view->GetModules().Add<Visibility::ViewModule>();
    view->GetModules().Add<SurfaceEvaluation::ViewModule>();
    view->GetModules().Add<Fsr3::ViewModule>();
    auto& upscaler = view->GetModules().Find<Fsr3::ViewModule>();

    const Vector2UInt initialSize(129, 73);
    const Vector2UInt resizedSize(131, 79);
    const Vector2UInt smallestSize(8, 8);
    const PackedVector4F initialColor(2.0f, 0.5f, 0.25f, 1.0f);
    const PackedVector4F changedColor(0.25f, 2.0f, 0.5f, 1.0f);

    Matrix4x4 previousCamera = view->GetViewProjectionMatrix();
    for (uint32_t frame = 0; frame < 7; ++frame)
    {
        SCOPED_TRACE(frame);

        if (frame == 2)
            upscaler.ResetHistory();

        if (frame == 4)
            upscaler.m_renderScale = 1.0f;

        if (frame == 1 || frame == 2)
            view->SetCameraTransform(Transform::Create(Vector3(static_cast<float>(frame), 0, 0), Quaternion::kIdentity, 1));

        Vector2UInt outputSize = initialSize;
        if (frame == 6)
            outputSize = smallestSize;
        else if (frame >= 3)
            outputSize = resizedSize;

        const PackedVector4F value = frame >= 2 ? changedColor : initialColor;

        queue->BeginFrame();
        graph->BeginFrame();
        auto& blackboard = graph->GetBlackboard();
        blackboard.Reset();

        // The pass skips itself until the module has prepared the view.
        Fsr3::AddPasses(*graph, blackboard);
        EXPECT_EQ(blackboard.TryGet<Fsr3::Output>(), nullptr);

        ViewData camera{};
        camera.m_viewProjection = view->GetViewProjectionMatrix();
        camera.m_unjitteredViewProjection = camera.m_viewProjection;
        camera.m_previousViewProjection = camera.m_viewProjection;
        camera.m_inverseViewProjection = view->GetInverseViewProjectionMatrix();

        ViewRenderSetup setup;
        setup.m_outputResolution = outputSize;
        setup.m_renderResolution = outputSize;
        setup.m_constants = &camera;
        upscaler.PrepareView(*graph, blackboard, setup);

        const Vector2UInt renderSize = setup.m_renderResolution;
        const uint32_t scaledWidth = static_cast<uint32_t>(static_cast<float>(outputSize.x) * upscaler.m_renderScale);
        EXPECT_EQ(renderSize.x, frame >= 4 ? outputSize.x : scaledWidth);
        EXPECT_TRUE(setup.m_needsMotionVectors);
        EXPECT_FALSE(Math::CmpEqual(camera.m_viewProjection, camera.m_unjitteredViewProjection, 0.0f));

        const bool resetsHistory = frame == 0 || frame == 2 || frame == 3 || frame == 4 || frame == 6;
        const Matrix4x4 expectedPrevious = resetsHistory ? camera.m_unjitteredViewProjection : previousCamera;
        EXPECT_TRUE(Math::CmpEqual(camera.m_previousViewProjection, expectedPrevious, 0.0f));
        previousCamera = camera.m_unjitteredViewProjection;

        const auto texture = [&](const Env::Name name, const Core::Format format) {
            return Core::Texture::Create(device.Get(), name, format, renderSize);
        };

        Rc color = texture("Fsr3ProbeColor", Core::Format::kR16G16B16A16_SFLOAT);
        Rc depth = texture("Fsr3ProbeDepth", Core::Format::kR32_SFLOAT);
        Rc motion = texture("Fsr3ProbeMotion", Core::Format::kR16G16_SFLOAT);
        Rc reactive = texture("Fsr3ProbeReactive", Core::Format::kR8_UNORM);

        auto& data = blackboard.Add<RendererViewData>();
        data.m_view = view;
        data.m_mainColorTarget = color.Get();
        data.m_mainDepthTarget = depth.Get();
        data.m_outputColorTarget = color.Get();
        blackboard.Add<SurfaceEvaluation::Output>().m_motion = motion;
        if (frame % 2 == 0)
            blackboard.Add<ForwardTransparency::Output>().m_reactive = reactive;

        if (frame == 2)
            blackboard.Add<Fsr3::Masks>().m_transparencyAndComposition = reactive;

        Fsr3Probe::FillConstants fill{};
        fill.m_color = graph->GetUAV(color.Get());
        fill.m_depth = graph->GetUAV(depth.Get());
        fill.m_motion = graph->GetUAV(motion.Get());
        fill.m_reactive = graph->GetUAV(reactive.Get());
        fill.m_size = renderSize;
        fill.m_value = value;
        graph->AddDispatch("FillFsr3Probe", fillPipeline, fill, Math::CeilDivide(renderSize, 8u));

        Fsr3::AddPasses(*graph, blackboard);
        const auto& output = blackboard.Get<Fsr3::Output>();
        EXPECT_EQ(output.m_color->GetDesc().GetSize2D(), outputSize);
        EXPECT_EQ(data.m_outputColorTarget, output.m_color.Get());

        const uint32_t pixelCount = outputSize.x * outputSize.y;
        const uint32_t bytes = pixelCount * sizeof(PackedVector4F);
        auto pixels = Core::Buffer::CreateStructured<PackedVector4F>(device.Get(), "Fsr3ProbePixels", pixelCount);
        Rc readback = Core::Buffer::CreateByteAddress(device.Get(), "Fsr3ProbeReadback", bytes);

        Core::ResourceCommitParams commit;
        commit.m_memory = Core::ResourceMemory::kHostRandomAccess;
        commit.m_bindFlags = Core::BarrierAccessFlags::kCopyDest;
        pool->CommitBufferMemory(readback.Get(), commit);

        Fsr3Probe::InspectConstants inspect{};
        inspect.m_color = graph->GetSRV(output.m_color.Get());
        inspect.m_pixels = graph->GetUAV(pixels.Get());
        inspect.m_size = outputSize;
        graph->AddDispatch("InspectFsr3Probe", inspectPipeline, inspect, Math::CeilDivide(outputSize, 8u));
        graph->AddCopyPass(Core::BufferView(readback.Get()), Core::BufferView(pixels.Get()));
        graph->CompileAndExecute();

        const auto fence = queue->GetCurrentFence();
        queue->SubmitFrame();
        fence.Wait();

        const auto* mapped = static_cast<const PackedVector4F*>(readback->Map());
        readback->InvalidateMappedRange(0, bytes);

        uint32_t invalidPixels = 0;
        for (uint32_t pixel = 0; pixel < pixelCount; ++pixel)
        {
            for (uint32_t channel = 0; channel < 3; ++channel)
            {
                const float actual = mapped[pixel].m_values[channel];
                if (!std::isfinite(actual) || std::abs(actual - value.m_values[channel]) > 0.04f)
                    ++invalidPixels;
            }
        }

        EXPECT_EQ(invalidPixels, 0u);

        readback->Unmap();
        pool->EndFrame();
        device->EndFrame();
    }

    graph.Reset();
    scene->DestroyView(view);
    renderer.DestroyScene(scene);
    Renderer::Shutdown();
    pipelines.Reset();
    device->WaitIdle();
}
