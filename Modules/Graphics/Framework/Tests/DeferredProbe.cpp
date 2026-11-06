#include "DeferredProbe.h"
#include <Graphics/Core/DescriptorManager.h>
#include <Graphics/Core/GraphicsQueue.h>
#include <Graphics/Core/PipelineFactory.h>
#include <Graphics/Core/ResourcePool.h>
#include <Graphics/Core/RingUploader.h>
#include <Graphics/Core/Vulkan/GraphicsQueue.h>
#include <Graphics/Features/Lighting/GlobalLightingSceneModule.h>
#include <Graphics/Passes/GBufferDebugPass.h>
#include <Graphics/Passes/RendererPassCommon.h>
#include <Graphics/Passes/SurfaceEvaluationPass.h>
#include <Graphics/Passes/SurfaceWorkBuilder.h>
#include <Graphics/Passes/VisibilityDebug.h>
#include <Graphics/Passes/VisibilityPass.h>
#include <Graphics/Scene/SceneImpl.h>
#include <Graphics/Scene/SurfaceEvaluation.h>
#include <gtest/gtest.h>

namespace FE::Graphics::DeferredProbe
{
    namespace
    {
        void CheckEmptyDebug(Core::Device* device, Core::ResourcePool* pool, Core::GraphicsQueue* queue,
                             Core::DescriptorManager* descriptors, Core::PipelineFactory* pipelines)
        {
            Rc<SceneImpl> scene = Memory::DefaultNew<SceneImpl>(nullptr);
            View* view = scene->CreateView();
            view->GetModules().Add<Visibility::ViewModule>();
            view->GetModules().Add<SurfaceEvaluation::ViewModule>();
            view->GetModules().Add<Visibility::DebugViewModule>();
            view->GetModules().Add<GBufferDebug::ViewModule>();
            const Vector2UInt resolution(33, 17);
            const uint32_t bytes = resolution.x * resolution.y * sizeof(uint32_t);
            Rc<Core::Texture> color =
                Core::Texture::Create(device, "EmptyDebugColor", Core::Format::kB10G11R11_UFLOAT, resolution);
            Rc<Core::Texture> depth = Core::Texture::Create(device, "EmptyDebugDepth", Core::Format::kD32_SFLOAT, resolution);
            auto stats = Core::Buffer::CreateStructured<uint32_t>(device, "EmptyDebugStats", bytes / sizeof(uint32_t));
            Rc<Core::Buffer> readback = Core::Buffer::CreateByteAddress(device, "EmptyDebugReadback", bytes);
            Core::ResourceCommitParams commit;
            commit.m_memory = Core::ResourceMemory::kHostRandomAccess;
            commit.m_bindFlags = Core::BarrierAccessFlags::kCopyDest;
            pool->CommitBufferMemory(readback.Get(), commit);
            Core::ComputePipelineRequest request;
            request.m_desc.SetComputeShader("Shaders/Tests/EmptyDebugProbe.cs.hlsl");
            Core::ComputePipeline* inspect = pipelines->CreateComputePipeline(request);
            inspect->GetCompletionWaitGroup()->Wait();
            ASSERT_TRUE(inspect->IsReady());
            Rc<Core::FrameGraph> graph = device->CreateFrameGraph(descriptors, pool, queue);
            Core::RingUploader uploader;
            uploader.Setup("EmptyDebugUploader", pool, 64 * 1024);

            for (const Env::Name mode :
                 { Env::Name("visibility"), Env::Name("normal"), Env::Name("depth-error"), Env::Name("evaluation-group") })
            {
                SCOPED_TRACE(mode.c_str());
                ASSERT_TRUE(view->SetDebugMode(mode));
                queue->BeginFrame();
                graph->BeginFrame();
                auto& blackboard = graph->GetBlackboard();
                blackboard.Reset();
                auto& data = blackboard.Add<RendererViewData>();
                data.m_scene = scene.Get();
                data.m_view = view;
                data.m_mainColorTarget = color.Get();
                data.m_mainDepthTarget = depth.Get();
                data.m_viewportRect = RectF(0, 0, 33, 17);
                data.m_renderQueueUploader = &uploader;
                blackboard.Add<SceneData*>(&scene->GetRenderData());
                blackboard.Add<SurfaceEvaluationInputs>();
                view->Update(blackboard);

                // Poison pooled output before exercising the installed stages with no object producers.
                auto* poison = graph->AllocatePassData<RendererTargetClearPassDesc>();
                poison->m_colorTarget = Core::TextureView::Create(color.Get());
                poison->m_depthTarget = Core::TextureView::Create(depth.Get());
                poison->m_viewport = data.m_viewportRect;
                graph->AddPass("PoisonDebugColor", poison, [](Core::FrameGraphContext& context) {
                    context.ClearColorTarget(0, Core::ColorClearValue::Float(Color4F(1, 0, 1, 1)));
                    context.ClearDepthStencilTarget(0, 0);
                    context.ClearRenderTargets();
                });
                Visibility::AddPasses(*graph, blackboard);
                Visibility::AddDebugPasses(*graph, blackboard);
                SurfaceEvaluation::AddPasses(*graph, blackboard);
                GBufferDebug::AddPasses(*graph, blackboard);

                Constants check{};
                check.m_color = graph->GetSRV(color.Get());
                check.m_stats = graph->GetUAV(stats.Get());
                check.m_resolution = resolution;
                graph->AddDispatch("InspectEmptyDebugColor", inspect, check, Math::CeilDivide(resolution, 8u));
                graph->AddCopyPass(Core::BufferView(readback.Get()), Core::BufferView(stats.Get()));
                graph->CompileAndExecute();
                uploader.CloseFrame(queue->GetCurrentFence());
                auto* commands = Vulkan::ImplCast(queue)->GetCurrentCommandBuffer();
                const auto completion = queue->CloseFrame();
                commands->EnqueueFenceToSignal(completion);
                commands->Submit();
                completion.Wait();
                const uint32_t* mismatches = static_cast<const uint32_t*>(readback->Map());
                readback->InvalidateMappedRange(0, bytes);
                uint32_t wrong = 0;
                for (uint32_t pixel = 0; pixel < resolution.x * resolution.y; ++pixel)
                    wrong += mismatches[pixel];

                EXPECT_EQ(wrong, 0u);
                readback->Unmap();
            }

            graph.Reset();
            uploader.Shutdown();
        }


    } // namespace


    void Check(Core::Device* device, Core::ResourcePool* pool, Core::GraphicsQueue* queue, Core::DescriptorManager* descriptors,
               Core::PipelineFactory* pipelines)
    {
        CheckEmptyDebug(device, pool, queue, descriptors, pipelines);

        const Vector2UInt resolution(257, 129);
        Rc<Core::FrameGraph> graph = device->CreateFrameGraph(descriptors, pool, queue);
        Core::RingUploader uploader;
        uploader.Setup("DeferredProbeUploader", pool, 256 * 1024);
        Rc<Core::Texture> planes[4];
        for (uint32_t plane = 0; plane < 4; ++plane)
            planes[plane] =
                Core::Texture::Create(device, Fmt::FormatName("DeferredProbeG{}", plane), Core::Format::kR32_UINT, resolution);

        Rc<Core::Texture> depth = Core::Texture::Create(device, "DeferredProbeDepth", Core::Format::kR32_SFLOAT, resolution);
        auto stats = Core::Buffer::CreateStructured<uint32_t>(device, "DeferredProbeStats", 4);
        Rc<Core::Buffer> readback = Core::Buffer::CreateByteAddress(device, "DeferredProbeReadback", 16);
        Rc<Core::Buffer> argsReadback = Core::Buffer::CreateByteAddress(device, "DeferredProbeArgsReadback", 36);
        Core::ResourceCommitParams commit;
        commit.m_memory = Core::ResourceMemory::kHostRandomAccess;
        commit.m_bindFlags = Core::BarrierAccessFlags::kCopyDest;
        pool->CommitBufferMemory(readback.Get(), commit);
        pool->CommitBufferMemory(argsReadback.Get(), commit);
        const auto pipeline = [&](const char* source, const Env::Name defines = Env::Name::kEmpty) {
            Core::ComputePipelineRequest request;
            request.m_desc.SetComputeShader(Env::Name(source));
            request.m_defines = defines;
            Core::ComputePipeline* result = pipelines->CreateComputePipeline(request);
            result->GetCompletionWaitGroup()->Wait();
            FE_Assert(result->IsReady());
            return result;
        };
        Core::ComputePipeline* seedPipeline = pipeline("Shaders/Tests/DeferredProbeSeed.cs.hlsl");
        Core::ComputePipeline* inspectPipeline = pipeline("Shaders/Tests/DeferredProbeInspect.cs.hlsl");
        Core::ComputePipeline* initializePipeline = pipeline("Shaders/Deferred/InitializeDeferred.cs.hlsl");
        ViewData view{};
        view.m_inverseViewProjection = Matrix4x4::kIdentity;
        view.m_inverseViewProjection.m_rows[3] = Vector4(0.1f, -0.2f, 0.3f, 1.5f);
        view.m_cameraPosition = PackedVector4F(0, 0, -2, 1);
        view.m_viewport = PackedVector4F(3, 5, 251, 119);
        view.m_backgroundColor = PackedVector4F(0, 0, 0, 1);
        const Visibility::SurfaceBinding bindings[] = { { 0, 0 }, { 1, 1 }, { 2, 2 } };
        Visibility::SurfaceWork work;
        auto routing = Visibility::CreateSurfaceRouting(device, bindings, 3, 3);
        for (uint32_t testCase = 0; testCase < 5; ++testCase)
        {
            SCOPED_TRACE(testCase);
            const bool empty = testCase == 2;
            const bool cameraProbe = testCase == 3;
            const bool wideHDR = testCase == 1 || cameraProbe;
            const uint32_t lanes = testCase == 1 ? 64 : 32;
            const Core::Format format = wideHDR ? Core::Format::kR16G16B16A16_SFLOAT : Core::Format::kB10G11R11_UFLOAT;
            if (cameraProbe)
            {
                const Matrix4x4 camera =
                    Matrix4x4::RotationX(0.7f) * Matrix4x4::RotationY(-1.1f) * Matrix4x4::Translation(Vector3(3, -2, 5));
                const Matrix4x4 projection = Matrix4x4::Projection(1.0f, 1.7f, 0.1f, 1000.0f);
                view.m_inverseViewProjection = Math::Invert(Math::InvertTransform(camera) * projection);
                view.m_cameraPosition = PackedVector4F(3, -2, 5, 1);
            }

            inspectPipeline = pipeline("Shaders/Tests/DeferredProbeInspect.cs.hlsl",
                                       cameraProbe   ? Env::Name("CAMERA_PROBE=1")
                                           : wideHDR ? Env::Name("WIDE_HDR=1")
                                                     : Env::Name::kEmpty);
            work = Visibility::CreateSurfaceWork(device, resolution, routing, lanes);
            Rc<Core::Texture> color = Core::Texture::Create(device, "DeferredProbeColor", format, resolution);
            Core::ComputePipeline* simple =
                pipeline("Shaders/Materials/ShadeSimpleLit.cs.hlsl", Env::Name(Fmt::Format("EVALUATION_LANES={}", lanes)));
            Core::ComputePipeline* pbr =
                pipeline("Shaders/Materials/ShadeStandardPBR.cs.hlsl", Env::Name(Fmt::Format("EVALUATION_LANES={}", lanes)));
            queue->BeginFrame();
            graph->BeginFrame();
            GlobalLightingParameters lighting;
            lighting.m_directionalLightOrientation = Quaternion(-0.77585731f, 0.44334703f, 0, 0.44887509f);
            lighting.m_directionalLightIlluminanceLux = testCase == 1 ? 27000.0f : 10000.0f;
            lighting.m_directionalLightColor = testCase == 1 ? PackedVector3F(0.3f, 0.7f, 1) : PackedVector3F(1, 1, 1);
            if (testCase == 1)
                lighting.m_directionalLightOrientation = Quaternion::RotationY(2.4f);

            const GlobalLightingData lightingData = testCase == 4 ? GlobalLightingData{} : lighting.BuildRenderData();
            const ConstantsPtr<GlobalLightingData> lightingConstants = graph->CreateConstants(lightingData);

            // The case with zero lighting data also covers a scene without global lighting.
            ViewLighting viewLighting{};
            viewLighting.m_global = testCase == 4 ? ConstantsPtr<GlobalLightingData>{} : lightingConstants;
            const ConstantsPtr<ViewLighting> viewLightingConstants = graph->CreateConstants(viewLighting);
            const ConstantsPtr<ViewData> viewConstants = graph->CreateConstants(view);
            Constants seed{};
            seed.m_g0 = graph->GetUAV(Core::TextureView::Create(planes[0].Get()));
            seed.m_g1 = graph->GetUAV(Core::TextureView::Create(planes[1].Get()));
            seed.m_g2 = graph->GetUAV(Core::TextureView::Create(planes[2].Get()));
            seed.m_g3 = graph->GetUAV(Core::TextureView::Create(planes[3].Get()));
            seed.m_depth = graph->GetUAV(Core::TextureView::Create(depth.Get()));
            seed.m_stats = graph->GetUAV(stats.Get());
            seed.m_resolution = resolution;
            seed.m_empty = empty;
            graph->AddDispatch("SeedDeferredProbe", seedPipeline, seed, Math::CeilDivide(resolution, 8u));
            Visibility::DeferredConstants initialize{};
            initialize.m_view = viewConstants;
            initialize.m_lighting = viewLightingConstants;
            initialize.m_g0 = graph->GetSRV(Core::TextureView::Create(planes[0].Get()));
            initialize.m_output = graph->GetUAV(Core::TextureView::Create(color.Get()));
            initialize.m_work.m_resolution = resolution;
            initialize.m_shaderCount = 3;
            graph->AddDispatch("InitializeDeferredProbe", initializePipeline, initialize, Math::CeilDivide(resolution, 8u));
            Visibility::BuildSurfaceWork(*graph, uploader, Visibility::ShadingWorkInput{ planes[0].Get() }, work);
            for (uint32_t slot = 0; slot < 3; ++slot)
            {
                Visibility::DeferredConstants shade{};
                shade.m_view = viewConstants;
                shade.m_lighting = viewLightingConstants;
                shade.m_g0 = graph->GetSRV(Core::TextureView::Create(planes[0].Get()));
                shade.m_g1 = graph->GetSRV(Core::TextureView::Create(planes[1].Get()));
                shade.m_g2 = graph->GetSRV(Core::TextureView::Create(planes[2].Get()));
                shade.m_g3 = graph->GetSRV(Core::TextureView::Create(planes[3].Get()));
                shade.m_depth = graph->GetSRV(Core::TextureView::Create(depth.Get()));
                shade.m_output = graph->GetUAV(Core::TextureView::Create(color.Get()));
                shade.m_work.m_pixels = graph->GetSRV(work.m_pixels.Get());
                shade.m_work.m_waves = graph->GetSRV(work.m_waves.Get());
                shade.m_work.m_ranges = graph->GetSRV(work.m_ranges.Get());
                shade.m_work.m_resolution = resolution;
                shade.m_work.m_tilesPerRow = 3;
                shade.m_work.m_shaderSlot = slot;
                graph->AddDispatchIndirect("ShadeDeferredProbe",
                                           slot == 0 ? simple : pbr,
                                           shade,
                                           Core::BufferView(work.m_arguments.Get()).Slice(slot * 12, 12));
            }

            Constants inspect{};
            inspect.m_view = viewConstants;
            inspect.m_expectedLighting = lightingConstants;
            inspect.m_color = graph->GetSRV(Core::TextureView::Create(color.Get()));
            inspect.m_stats = graph->GetUAV(stats.Get());
            inspect.m_resolution = resolution;
            inspect.m_empty = empty;
            graph->AddDispatch("InspectDeferredProbe", inspectPipeline, inspect, Math::CeilDivide(resolution, 8u));
            graph->AddCopyPass(Core::BufferView(readback.Get()), Core::BufferView(stats.Get()));
            graph->AddCopyPass(Core::BufferView(argsReadback.Get()), Core::BufferView(work.m_arguments.Get()));
            graph->CompileAndExecute();
            uploader.CloseFrame(queue->GetCurrentFence());
            Vulkan::CommandBuffer* native = Vulkan::ImplCast(queue)->GetCurrentCommandBuffer();
            const Core::FenceSyncPoint fence = queue->CloseFrame();
            native->EnqueueFenceToSignal(fence);
            native->Submit();
            fence.Wait();
            device->WaitIdle();
            const uint32_t* result = static_cast<const uint32_t*>(readback->Map());
            readback->InvalidateMappedRange(0, 16);
            EXPECT_EQ(result[0], 0u);
            EXPECT_LT(std::bit_cast<float>(result[1]), wideHDR ? 0.0012f : 0.032f);
            EXPECT_EQ(result[2], resolution.x * resolution.y);
            EXPECT_EQ(result[3], 0u);
            readback->Unmap();
            const uint32_t* args = static_cast<const uint32_t*>(argsReadback->Map());
            argsReadback->InvalidateMappedRange(0, 36);
            EXPECT_EQ(args[3], 0u);
            if (empty)
            {
                EXPECT_EQ(args[0], 0u);
                EXPECT_EQ(args[6], 0u);
            }

            argsReadback->Unmap();
        }

        graph.Reset();
        uploader.Shutdown();
    }
} // namespace FE::Graphics::DeferredProbe
