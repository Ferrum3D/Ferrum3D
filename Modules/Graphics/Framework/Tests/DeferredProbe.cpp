#include "DeferredProbe.h"
#include <Graphics/Core/DescriptorManager.h>
#include <Graphics/Core/GraphicsQueue.h>
#include <Graphics/Core/PipelineFactory.h>
#include <Graphics/Core/ResourcePool.h>
#include <Graphics/Core/RingUploader.h>
#include <Graphics/Core/Vulkan/GraphicsQueue.h>
#include <Graphics/Passes/SurfaceWorkBuilder.h>
#include <gtest/gtest.h>

namespace FE::Graphics::DeferredProbe
{
    void Check(Core::Device* device, Core::ResourcePool* pool, Core::GraphicsQueue* queue, Core::DescriptorManager* descriptors,
               Core::PipelineFactory* pipelines)
    {
        const Vector2UInt resolution(257, 129);
        Rc<Core::FrameGraph> graph = device->CreateFrameGraph(descriptors, pool, queue);
        Core::RingUploader uploader;
        uploader.Setup("DeferredProbeUploader", pool, 256 * 1024);
        Rc<Core::Texture> planes[4];
        for (uint32_t plane = 0; plane < 4; ++plane)
            planes[plane] =
                Core::Texture::Create(device, Fmt::FormatName("DeferredProbeG{}", plane), Core::Format::kR32_UINT, resolution);

        Rc<Core::Texture> depth = Core::Texture::Create(device, "DeferredProbeDepth", Core::Format::kR32_SFLOAT, resolution);
        Rc<Core::Buffer> viewBuffer = Core::Buffer::CreateStructured<ViewData>(device, "DeferredProbeView", 1);
        Rc<Core::Buffer> stats = Core::Buffer::CreateByteAddress(device, "DeferredProbeStats", 16);
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
        view.m_backgroundColor = PackedVector4F(0.1f, 0.2f, 0.3f, 1);
        const Visibility::SurfaceBinding bindings[] = { { 0, 0 }, { 1, 1 }, { 2, 2 } };
        Visibility::SurfaceWork work;
        auto routing = Visibility::CreateSurfaceRouting(device, bindings, 3, 3);
        for (uint32_t testCase = 0; testCase < 4; ++testCase)
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
            FE_Verify(uploader.Upload(*graph, viewBuffer.Get(), view));
            auto* seed = graph->AllocatePassData<PassDesc>();
            seed->m_pipeline = seedPipeline;
            seed->m_constants.m_g0 = graph->GetUAV(Core::TextureView::Create(planes[0].Get()));
            seed->m_constants.m_g1 = graph->GetUAV(Core::TextureView::Create(planes[1].Get()));
            seed->m_constants.m_g2 = graph->GetUAV(Core::TextureView::Create(planes[2].Get()));
            seed->m_constants.m_g3 = graph->GetUAV(Core::TextureView::Create(planes[3].Get()));
            seed->m_constants.m_depth = graph->GetUAV(Core::TextureView::Create(depth.Get()));
            seed->m_constants.m_stats = graph->GetUAV(stats.Get());
            seed->m_constants.m_resolution = resolution;
            seed->m_constants.m_empty = empty;
            graph->AddDispatchPass("SeedDeferredProbe", seed, Math::CeilDivide(resolution, 8u));
            auto* initialize = graph->AllocatePassData<Visibility::DeferredDirectPassDesc>();
            initialize->m_pipeline = initializePipeline;
            initialize->m_constants.m_view = graph->GetSRV(viewBuffer.Get());
            initialize->m_constants.m_g0 = graph->GetSRV(Core::TextureView::Create(planes[0].Get()));
            initialize->m_constants.m_output = graph->GetUAV(Core::TextureView::Create(color.Get()));
            initialize->m_constants.m_resolution = resolution;
            initialize->m_constants.m_shaderCount = 3;
            graph->AddDispatchPass("InitializeDeferredProbe", initialize, Math::CeilDivide(resolution, 8u));
            Visibility::BuildSurfaceWork(*graph, uploader, nullptr, nullptr, planes[0].Get(), work, true);
            for (uint32_t slot = 0; slot < 3; ++slot)
            {
                auto* shade = graph->AllocatePassData<Visibility::DeferredPassDesc>();
                shade->m_pipeline = slot == 0 ? simple : pbr;
                shade->m_arguments = Core::BufferView(work.m_arguments.Get()).Slice(slot * 12, 12);
                shade->m_constants.m_view = graph->GetSRV(viewBuffer.Get());
                shade->m_constants.m_g0 = graph->GetSRV(Core::TextureView::Create(planes[0].Get()));
                shade->m_constants.m_g1 = graph->GetSRV(Core::TextureView::Create(planes[1].Get()));
                shade->m_constants.m_g2 = graph->GetSRV(Core::TextureView::Create(planes[2].Get()));
                shade->m_constants.m_g3 = graph->GetSRV(Core::TextureView::Create(planes[3].Get()));
                shade->m_constants.m_depth = graph->GetSRV(Core::TextureView::Create(depth.Get()));
                shade->m_constants.m_output = graph->GetUAV(Core::TextureView::Create(color.Get()));
                shade->m_constants.m_pixels = graph->GetSRV(work.m_pixels.Get());
                shade->m_constants.m_waves = graph->GetSRV(work.m_waves.Get());
                shade->m_constants.m_ranges = graph->GetSRV(work.m_ranges.Get());
                shade->m_constants.m_resolution = resolution;
                shade->m_constants.m_tilesPerRow = 3;
                shade->m_constants.m_shaderSlot = slot;
                graph->AddPass("ShadeDeferredProbe", shade, [](Core::FrameGraphContext& context) {
                    context.DispatchIndirect();
                });
            }

            auto* inspect = graph->AllocatePassData<PassDesc>();
            inspect->m_pipeline = inspectPipeline;
            inspect->m_constants.m_view = graph->GetSRV(viewBuffer.Get());
            inspect->m_constants.m_color = graph->GetSRV(Core::TextureView::Create(color.Get()));
            inspect->m_constants.m_stats = graph->GetUAV(stats.Get());
            inspect->m_constants.m_resolution = resolution;
            inspect->m_constants.m_empty = empty;
            graph->AddDispatchPass("InspectDeferredProbe", inspect, Math::CeilDivide(resolution, 8u));
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
