#include "RasterProbe.h"
#include <Core/Math/Packing.h>
#include <Graphics/Core/DescriptorManager.h>
#include <Graphics/Core/GraphicsQueue.h>
#include <Graphics/Core/PipelineFactory.h>
#include <Graphics/Core/PipelineVariantSet.h>
#include <Graphics/Core/ResourcePool.h>
#include <Graphics/Core/RingUploader.h>
#include <Graphics/Core/Vulkan/GraphicsQueue.h>
#include <Graphics/Passes/SurfaceWorkBuilder.h>
#include <Graphics/Passes/VisibilityPass.h>
#include <Shaders/Passes/MeshPass/IndirectArguments.h>
#include <Shaders/Passes/MeshPass/MeshPass.h>
#include <Shaders/Passes/Visibility/Surface.h>
#include <gtest/gtest.h>

namespace FE::Graphics::RasterProbe
{
    namespace
    {
        struct Vertex final
        {
            PackedVector3F m_position;
            uint32_t m_uv;
            uint32_t m_color = 0;
            uint32_t m_normal;
            uint32_t m_tangent;
            uint32_t m_weights = 0;
            uint32_t m_indices[2]{};
        };


        struct Geometry final
        {
            Vertex m_vertices[3];
            uint32_t m_remap[3] = { 2, 0, 1 };
            Core::MeshletHeader m_headers[2] = { {}, Core::MeshletHeader::Pack(3, 0, 1, 1) };
            Core::PackedTriangle m_triangles[2] = { {}, Core::PackedTriangle::Pack(1, 2, 0) };
            PackedVector4F m_spheres[2] = { PackedVector4F(10, 0, 0.5f, 0.1f), PackedVector4F(0, 0, 0.5f, 2) };
        };

        static_assert(sizeof(Vertex) == 40);
        static_assert(sizeof(Geometry) == 188);
    } // namespace


    void Check(Core::Device* device, Core::ResourcePool* pool, Core::GraphicsQueue* queue, Core::DescriptorManager* descriptors,
               Core::PipelineFactory* pipelines)
    {
        Core::CompileGlobalPipelineSets(pipelines);
        Core::WaitForGlobalPipelineSets();
        DB::Database database(device, pool);
        MeshInstanceTable instances(&database);
        MeshGroupTable groups(&database);
        MeshLodInfoTable lods(&database);
        MaterialInstanceTable materials(&database);
        const auto material = materials.AllocateRow();
        materials.WriteRow(material).m_evaluationSlot.Get() = 0;
        materials.WriteRow(material).m_shadingSlot.Get() = 0;
        const auto unusedInstance = instances.AllocateRow();
        const auto instance = instances.AllocateRow();
        const auto group = groups.AllocateRow();
        const auto lod = lods.AllocateRow();
        EXPECT_NE(unusedInstance.m_rowIndex, instance.m_rowIndex);
        EXPECT_GT(instance.m_rowIndex, 0u);

        Geometry geometry;
        const uint32_t normal = Math::Pack::RGBA32FloatToA2R10G10B10Unorm(Vector4(0.5f, 0.5f, 1.0f, 0.0f));
        const uint32_t tangent = Math::Pack::RGBA32FloatToA2R10G10B10Unorm(Vector4(1.0f, 0.5f, 0.5f, 0.0f));
        geometry.m_vertices[0] = { PackedVector3F(-0.8f, -1.6f, 0.5f),
                                   Math::Pack::RG32FloatToRG16Float(Vector2(0, 0)),
                                   0,
                                   normal,
                                   tangent };
        geometry.m_vertices[1] = { PackedVector3F(0.8f, -1.6f, 0.5f),
                                   Math::Pack::RG32FloatToRG16Float(Vector2(1, 0)),
                                   0,
                                   normal,
                                   tangent };
        geometry.m_vertices[2] = { PackedVector3F(0, 1.6f, 0.5f),
                                   Math::Pack::RG32FloatToRG16Float(Vector2(0, 1)),
                                   0,
                                   normal,
                                   tangent };
        Rc<Core::Buffer> geometryBuffer = Core::Buffer::CreateByteAddress(device, "RasterProbeGeometry", sizeof(geometry));
        Core::ResourceCommitParams geometryCommit;
        geometryCommit.m_memory = Core::ResourceMemory::kDeviceLocal;
        geometryCommit.m_bindFlags = Core::BarrierAccessFlags::kShaderRead | Core::BarrierAccessFlags::kCopyDest;
        pool->CommitBufferMemory(geometryBuffer.Get(), geometryCommit);
        const auto groupRow = groups.WriteRow(group);
        groupRow.m_geometry.Get() = BufferPointer(geometryBuffer->GetDeviceAddress());
        groupRow.m_lods.Get() = { lod.m_rowIndex, 1 };
        groupRow.m_materialInstance.Get() = material;
        lods.WriteRow(lod).m_info.Get() = { 3, 3, 2, 2 };
        const auto instanceRow = instances.WriteRow(instance);
        instanceRow.m_meshGroup.Get() = group;
        instanceRow.m_transform.Get() = Matrix4x4::Scale(Vector3(-1, 0.5f, 1));

        Rc<Core::FrameGraph> graph = device->CreateFrameGraph(descriptors, pool, queue);
        Core::RingUploader uploader;
        uploader.Setup("RasterProbeUpload", pool, 256 * 1024);
        Rc<Core::Buffer> viewBuffer = Core::Buffer::CreateStructured<MeshPass::ViewData>(device, "RasterProbeView", 1);
        Rc<Core::Buffer> visible = Core::Buffer::CreateStructured<MeshPass::VisibleInstance>(device, "RasterProbeInstance", 1);
        Rc<Core::Buffer> work = Core::Buffer::CreateStructured<MeshPass::MeshletWorkChunk>(device, "RasterProbeWork", 1);
        Rc<Core::Buffer> commands = Core::Buffer::CreateStructured<MeshPass::MeshCommandRange>(device, "RasterProbeCommands", 1);
        Rc<Core::Buffer> arguments =
            Core::Buffer::CreateStructured<MeshPass::MeshDispatchArguments>(device, "RasterProbeArguments", 1);
        Rc<Core::Buffer> output = Core::Buffer::CreateByteAddress(device, "RasterProbeOutput", 128);
        Rc<Core::Buffer> readback = Core::Buffer::CreateByteAddress(device, "RasterProbeReadback", 128);
        Core::ResourceCommitParams readbackCommit;
        readbackCommit.m_memory = Core::ResourceMemory::kHostRandomAccess;
        readbackCommit.m_bindFlags = Core::BarrierAccessFlags::kCopyDest;
        pool->CommitBufferMemory(readback.Get(), readbackCommit);
        Rc<Core::Texture> visibility =
            Core::Texture::Create(device, "RasterProbeVisibility", Core::Format::kR32G32_UINT, Vector2UInt(64, 64));
        Rc<Core::Texture> depth =
            Core::Texture::Create(device, "RasterProbeDepth", Core::Format::kD32_SFLOAT, Vector2UInt(64, 64));
        Core::GraphicsPipelineRequest coverageRequest;
        coverageRequest.m_desc.SetAmplificationShader("Shaders/Passes/MeshPass/MeshCull.as.hlsl")
            .SetMeshShader("Shaders/Passes/Visibility/DepthVisibility.ms.hlsl")
            .SetPixelShader("Shaders/Passes/Visibility/DepthVisibility.ps.hlsl")
            .SetRTVFormat(Core::Format::kR32G32_UINT)
            .SetDSVFormat(Core::Format::kD32_SFLOAT)
            .SetDepthStencil(Core::DepthStencilState::kWriteIfGreater);
        Core::GraphicsPipeline* coverage = pipelines->CreateGraphicsPipeline(coverageRequest);
        coverage->GetCompletionWaitGroup()->Wait();
        ASSERT_TRUE(coverage->IsReady());
        Core::ComputePipelineRequest probeRequest;
        probeRequest.m_desc.SetComputeShader("Shaders/Tests/RasterProbe.cs.hlsl");
        Core::ComputePipeline* probe = pipelines->CreateComputePipeline(probeRequest);
        probe->GetCompletionWaitGroup()->Wait();
        ASSERT_TRUE(probe->IsReady());

        queue->BeginFrame();
        graph->BeginFrame();
        FE_Verify(uploader.Upload(*graph, geometryBuffer.Get(), geometry));
        database.Update(*graph, queue->GetCurrentFence());
        auto* geometryAccess = graph->AllocatePassData<Core::BufferAccessPassDesc>();
        geometryAccess->m_access = { geometryBuffer.Get(),
                                     Core::BarrierSyncFlags::kAllShading,
                                     Core::BarrierAccessFlags::kShaderRead };
        graph->AddPass("RasterProbeGeometryReady", geometryAccess);
        FE_Verify(uploader.Upload(*graph, visible.Get(), MeshPass::VisibleInstance{ instance.m_rowIndex, lod.m_rowIndex }));
        FE_Verify(uploader.Upload(*graph, work.Get(), MeshPass::MeshletWorkChunk{ 0, 0 }));
        FE_Verify(uploader.Upload(*graph, commands.Get(), MeshPass::MeshCommandRange{ 0, 1 }));
        FE_Verify(uploader.Upload(*graph, arguments.Get(), MeshPass::MeshDispatchArguments{ 1, 1, 1 }));
        MeshPass::ViewData view{};
        view.m_viewProjection = Matrix4x4::kIdentity;
        view.m_instances = instances.GetDeviceAddress();
        view.m_groups = groups.GetDeviceAddress();
        view.m_lods = lods.GetDeviceAddress();
        view.m_materials = materials.GetDeviceAddress();
        view.m_visibleInstances = graph->GetSRV(visible.Get());
        view.m_workChunks = graph->GetSRV(work.Get());
        view.m_commands = graph->GetSRV(commands.Get());
        FE_Verify(uploader.Upload(*graph, viewBuffer.Get(), view));
        auto* targets = graph->AllocatePassData<Visibility::CoveragePassDesc>();
        targets->m_visibility = Core::TextureView::Create(visibility.Get());
        targets->m_depth = Core::TextureView::Create(depth.Get());
        targets->m_viewport = RectF(0, 0, 64, 64);
        graph->AddPass("ClearRasterProbe", targets, [](Core::FrameGraphContext& context) {
            context.ClearColorTarget(0, Core::ColorClearValue::Uint(0));
            context.ClearDepthStencilTarget(0, 0);
            context.ClearRenderTargets();
        });
        auto* mesh = graph->AllocatePassData<MeshPass::PassDesc>();
        constexpr auto shading = Core::BarrierSyncFlags::kAllShading;
        mesh->m_view = { viewBuffer.Get(), shading, Core::BarrierAccessFlags::kShaderRead };
        mesh->m_visibleInstances = { visible.Get(), shading, Core::BarrierAccessFlags::kShaderRead };
        mesh->m_workChunks = { work.Get(), shading, Core::BarrierAccessFlags::kShaderRead };
        mesh->m_commands = { commands.Get(), shading, Core::BarrierAccessFlags::kShaderRead };
        mesh->m_arguments = Core::BufferView(arguments.Get());
        const BufferSRVDescriptor viewDescriptor = graph->GetSRV(viewBuffer.Get());
        graph->AddPass("RasterProbeCoverage",
                       mesh,
                       graph->AddBasePassDesc(targets),
                       [coverage, viewDescriptor](Core::FrameGraphContext& context) {
                           context.SetPipeline(coverage);
                           context.PushConstants(MeshPass::Constants{ viewDescriptor, 0 });
                           context.DispatchMeshIndirect();
                       });
        Rc<Core::Texture> planes[4];
        for (uint32_t plane = 0; plane < 4; ++plane)
            planes[plane] = Core::Texture::Create(device,
                                                  Fmt::FormatName("RasterSurfaceG{}", plane),
                                                  Core::Format::kR32_UINT,
                                                  Vector2UInt(64, 64));

        Core::ComputePipelineRequest clearRequest;
        clearRequest.m_desc.SetComputeShader("Shaders/Passes/Visibility/ClearSurface.cs.hlsl");
        Core::ComputePipeline* clearPipeline = pipelines->CreateComputePipeline(clearRequest);
        clearPipeline->GetCompletionWaitGroup()->Wait();
        auto* clear = graph->AllocatePassData<Visibility::SurfaceDirectPassDesc>();
        clear->m_pipeline = clearPipeline;
        clear->m_constants.m_resolution = Vector2UInt(64, 64);
        clear->m_constants.m_g0 = graph->GetUAV(Core::TextureView::Create(planes[0].Get()));
        graph->AddDispatchPass("ClearRasterSurface", clear, Core::ComputeWorkGroupCount(Vector2UInt(8, 8)));
        const Visibility::SurfaceBinding binding{ group.m_rowIndex, 0 };
        auto surfaceWork = Visibility::CreateSurfaceWork(device,
                                                         Vector2UInt(64, 64),
                                                         festd::span(&binding, 1),
                                                         groups.GetReservedRowCount(),
                                                         1,
                                                         32);
        Visibility::BuildSurfaceWork(*graph, uploader, viewBuffer.Get(), visibility.Get(), planes[0].Get(), surfaceWork);
        Core::ComputePipelineRequest evaluateRequest;
        evaluateRequest.m_desc.SetComputeShader("Shaders/Materials/EvaluateBunnyVariants.cs.hlsl");
        evaluateRequest.m_defines = Env::Name("BUNNY_PALETTE=1");
        Core::ComputePipeline* evaluate = pipelines->CreateComputePipeline(evaluateRequest);
        evaluate->GetCompletionWaitGroup()->Wait();
        ASSERT_TRUE(evaluate->IsReady());
        auto* evaluation = graph->AllocatePassData<Visibility::SurfacePassDesc>();
        evaluation->m_pipeline = evaluate;
        evaluation->m_arguments = Core::BufferView(surfaceWork.m_arguments.Get()).Slice(0, 12);
        evaluation->m_constants.m_view = graph->GetSRV(viewBuffer.Get());
        evaluation->m_constants.m_visibility = graph->GetSRV(Core::TextureView::Create(visibility.Get()));
        evaluation->m_constants.m_pixels = graph->GetSRV(surfaceWork.m_pixels.Get());
        evaluation->m_constants.m_waves = graph->GetSRV(surfaceWork.m_waves.Get());
        evaluation->m_constants.m_ranges = graph->GetSRV(surfaceWork.m_ranges.Get());
        evaluation->m_constants.m_g0 = graph->GetUAV(Core::TextureView::Create(planes[0].Get()));
        evaluation->m_constants.m_g1 = graph->GetUAV(Core::TextureView::Create(planes[1].Get()));
        evaluation->m_constants.m_g2 = graph->GetUAV(Core::TextureView::Create(planes[2].Get()));
        evaluation->m_constants.m_g3 = graph->GetUAV(Core::TextureView::Create(planes[3].Get()));
        evaluation->m_constants.m_resolution = Vector2UInt(64, 64);
        evaluation->m_constants.m_tilesPerRow = 1;
        graph->AddPass("EvaluateRasterSurface", evaluation, [](Core::FrameGraphContext& context) {
            context.DispatchIndirect();
        });
        auto* inspect = graph->AllocatePassData<PassDesc>();
        inspect->m_pipeline = probe;
        inspect->m_constants.m_view = graph->GetSRV(viewBuffer.Get());
        inspect->m_constants.m_visibility = graph->GetSRV(Core::TextureView::Create(visibility.Get()));
        inspect->m_constants.m_depth = graph->GetSRV(Core::TextureView::Create(depth.Get()));
        inspect->m_constants.m_g0 = graph->GetSRV(Core::TextureView::Create(planes[0].Get()));
        inspect->m_constants.m_g1 = graph->GetSRV(Core::TextureView::Create(planes[1].Get()));
        inspect->m_constants.m_g2 = graph->GetSRV(Core::TextureView::Create(planes[2].Get()));
        inspect->m_constants.m_words = graph->GetUAV(output.Get());
        graph->AddDispatchPass("InspectRasterProbe", inspect, Core::ComputeWorkGroupCount(2));
        graph->AddCopyPass(Core::BufferView(readback.Get()), Core::BufferView(output.Get()));
        graph->CompileAndExecute();
        uploader.CloseFrame(queue->GetCurrentFence());
        Vulkan::CommandBuffer* nativeCommands = Vulkan::ImplCast(queue)->GetCurrentCommandBuffer();
        const Core::FenceSyncPoint fence = queue->CloseFrame();
        nativeCommands->EnqueueFenceToSignal(fence);
        nativeCommands->Submit();
        fence.Wait();
        device->WaitIdle();
        const uint32_t* words = static_cast<const uint32_t*>(readback->Map());
        readback->InvalidateMappedRange(0, 128);
        EXPECT_EQ(words[0], instance.m_rowIndex + 1);
        EXPECT_EQ(words[1], 64u);
        EXPECT_NEAR(std::bit_cast<float>(words[2]), 0.5f, 1e-6f);
        EXPECT_EQ(words[3], 1u);
        EXPECT_NEAR(std::bit_cast<float>(words[4]), 0.2451171875f, 1e-5f);
        EXPECT_NEAR(std::bit_cast<float>(words[5]), 0.490234375f, 1e-5f);
        EXPECT_NEAR(std::bit_cast<float>(words[6]), 1.0f, 1e-4f);
        EXPECT_NEAR(std::bit_cast<float>(words[7]), -1.0f, 1e-4f);
        EXPECT_EQ(std::bit_cast<float>(words[8]), -1.0f);
        EXPECT_NEAR(std::bit_cast<float>(words[9]), -0.01953125f, 1e-5f);
        EXPECT_NEAR(std::bit_cast<float>(words[10]), -0.01953125f, 1e-5f);
        EXPECT_NEAR(std::bit_cast<float>(words[11]), 0.5f, 1e-6f);
        EXPECT_EQ(words[12], 1u);
        EXPECT_EQ(words[13], 0x01f3da89u);
        EXPECT_GT(std::bit_cast<float>(words[14]), 0.9999f);
        EXPECT_EQ(words[15], 0u);
        EXPECT_EQ(words[16], 0u);
        EXPECT_EQ(words[18], 0u);
        EXPECT_EQ(words[19], 0u);
        EXPECT_EQ(words[28], 0u);
        readback->Unmap();
        graph.Reset();
        uploader.Shutdown();
    }
} // namespace FE::Graphics::RasterProbe
