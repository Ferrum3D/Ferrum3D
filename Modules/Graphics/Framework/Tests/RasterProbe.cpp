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
#include <Shaders/Deferred/Surface.h>
#include <Shaders/Passes/MeshPass/GpuWork.h>
#include <Shaders/Passes/MeshPass/IndirectArguments.h>
#include <Shaders/Passes/MeshPass/MeshPass.h>
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
               Core::PipelineFactory* pipelines, const uint32_t waveLaneCount)
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
        instanceRow.m_transform.Get() = Matrix3x4::FromMatrix4x4(Matrix4x4::Scale(Vector3(-1, 0.5f, 1)));
        Matrix4x4 previousTransform = Matrix4x4::Scale(Vector3(-1, 0.5f, 1));
        previousTransform.m_30 = 0.2f;
        previousTransform.m_31 = -0.4f;
        instanceRow.m_previousTransform.Get() = Matrix3x4::FromMatrix4x4(previousTransform);

        Rc<Core::FrameGraph> graph = device->CreateFrameGraph(descriptors, pool, queue);
        Core::RingUploader uploader;
        uploader.Setup("RasterProbeUpload", pool, 256 * 1024);
        const auto classifier = [&](const Env::Name path) {
            Core::ComputePipelineRequest request;
            request.m_desc.SetComputeShader(path);
            Core::ComputePipeline* result = pipelines->CreateComputePipeline(request);
            result->GetCompletionWaitGroup()->Wait();
            return result;
        };
        Core::ComputePipeline* countPipeline = classifier("Shaders/Passes/MeshPass/CountMeshSurfacePixels.cs.hlsl");
        Core::ComputePipeline* scatterPipeline = classifier("Shaders/Passes/MeshPass/ScatterMeshSurfacePixels.cs.hlsl");
        auto meshlets = Core::Buffer::CreateStructured<MeshPass::VisibleMeshlet>(device, "RasterProbeMeshlets", 1);
        auto commands = Core::Buffer::CreateStructured<MeshPass::MeshCommandRange>(device, "RasterProbeCommands", 1);
        auto arguments = Core::Buffer::CreateStructured<MeshPass::MeshDispatchArguments>(device, "RasterProbeArguments", 1);
        auto output = Core::Buffer::CreateStructured<uint32_t>(device, "RasterProbeOutput", 36);
        Rc<Core::Buffer> readback = Core::Buffer::CreateByteAddress(device, "RasterProbeReadback", 144);
        Core::ResourceCommitParams readbackCommit;
        readbackCommit.m_memory = Core::ResourceMemory::kHostRandomAccess;
        readbackCommit.m_bindFlags = Core::BarrierAccessFlags::kCopyDest;
        pool->CommitBufferMemory(readback.Get(), readbackCommit);
        Rc<Core::Texture> visibility =
            Core::Texture::Create(device, "RasterProbeVisibility", Core::Format::kR32G32_UINT, Vector2UInt(64, 64));
        Rc<Core::Texture> depth =
            Core::Texture::Create(device, "RasterProbeDepth", Core::Format::kD32_SFLOAT, Vector2UInt(64, 64));
        Core::GraphicsPipelineRequest coverageRequest;
        coverageRequest.m_desc.SetMeshShader("Shaders/Passes/Visibility/DepthVisibility.ms.hlsl")
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
        // Meshlet zero lies outside the view; compute culling passes only meshlet one to rasterization.
        FE_Verify(uploader.Upload(*graph, meshlets.Get(), MeshPass::VisibleMeshlet{ instance.m_rowIndex, 1 }));
        FE_Verify(uploader.Upload(*graph, commands.Get(), MeshPass::MeshCommandRange{ 0, 1 }));
        FE_Verify(uploader.Upload(*graph, arguments.Get(), MeshPass::MeshDispatchArguments{ 1, 1, 1 }));
        MeshPass::MeshViewData view{};
        MeshPass::MeshSceneData scene{};
        view.m_view.m_viewProjection = Matrix4x4::kIdentity;
        // A nonsymmetric projection that preserves this triangle's clip positions exposes transposed matrix loads.
        view.m_view.m_viewProjection.m_rows[2].x = 0.4f;
        view.m_view.m_viewProjection.m_rows[3].x = -0.2f;
        view.m_view.m_unjitteredViewProjection = view.m_view.m_viewProjection;
        view.m_view.m_previousViewProjection = view.m_view.m_viewProjection;
        scene.m_visibilityType = 7;
        scene.m_instances = instances.GetInstance();
        scene.m_groups = groups.GetInstance();
        scene.m_lods = lods.GetInstance();
        scene.m_materials = materials.GetInstance();
        view.m_scene = graph->CreateConstants(scene);
        const ConstantsPtr<MeshPass::MeshViewData> viewConstants = graph->CreateConstants(view);

        MeshPass::MeshPassData passData{};
        passData.m_visibleMeshlets = graph->GetSRV(meshlets.Get());
        passData.m_commands = graph->GetSRV(commands.Get());
        const ConstantsPtr<MeshPass::MeshPassData> passConstants = graph->CreateConstants(passData);

        // Shaders reach the geometry through a table row, like the database pages.
        auto* geometryAccess = graph->AllocatePassData<Core::BufferAccessPassDesc>();
        geometryAccess->m_access = { geometryBuffer.Get(),
                                     Core::BarrierSyncFlags::kAllShading,
                                     Core::BarrierAccessFlags::kShaderRead };
        graph->AddPass("RasterProbeGeometryAccess", geometryAccess);

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
        mesh->m_pipeline = coverage;
        mesh->m_constants = { viewConstants, passConstants, 0 };
        mesh->m_arguments = Core::BufferView(arguments.Get());
        graph->AddPass("RasterProbeCoverage", mesh, graph->AddBasePassDesc(targets), [](Core::FrameGraphContext& context) {
            context.DispatchMeshIndirect();
        });
        Rc<Core::Texture> planes[4];
        Rc motion = Core::Texture::Create(device, "RasterMotion", Core::Format::kR16G16_SFLOAT, { 64, 64 });
        for (uint32_t plane = 0; plane < 4; ++plane)
            planes[plane] = Core::Texture::Create(device,
                                                  Fmt::FormatName("RasterSurfaceG{}", plane),
                                                  Core::Format::kR32_UINT,
                                                  Vector2UInt(64, 64));

        Core::ComputePipelineRequest clearRequest;
        clearRequest.m_desc.SetComputeShader("Shaders/Deferred/ClearSurface.cs.hlsl");
        Core::ComputePipeline* clearPipeline = pipelines->CreateComputePipeline(clearRequest);
        clearPipeline->GetCompletionWaitGroup()->Wait();
        Visibility::SurfaceConstants clear{};
        clear.m_visibility = graph->GetSRV(visibility.Get());
        clear.m_work.m_resolution = Vector2UInt(64, 64);
        clear.m_g0 = graph->GetUAV(Core::TextureView::Create(planes[0].Get()));
        clear.m_motion = graph->GetUAV(motion.Get());
        graph->AddDispatch("ClearRasterSurface", clearPipeline, clear, Math::CeilDivide(clear.m_work.m_resolution, 8u));
        const Visibility::SurfaceBinding binding{ group.m_rowIndex, 0 };
        auto routing = Visibility::CreateSurfaceRouting(device, festd::span(&binding, 1), groups.GetReservedRowCount(), 1);
        auto surfaceWork = Visibility::CreateSurfaceWork(device, Vector2UInt(64, 64), routing, waveLaneCount);
        const Visibility::EvaluationWorkInput input{ viewConstants, visibility.Get(), countPipeline, scatterPipeline };
        Visibility::BuildSurfaceWork(*graph, uploader, input, surfaceWork);
        Core::ComputePipelineRequest evaluateRequest;
        evaluateRequest.m_desc.SetComputeShader("Shaders/Materials/EvaluateBunnyVariants.cs.hlsl");
        evaluateRequest.m_defines = Env::Name(Fmt::Format("BUNNY_PALETTE=1 EVALUATION_LANES={}", waveLaneCount));
        Core::ComputePipeline* evaluate = pipelines->CreateComputePipeline(evaluateRequest);
        evaluate->GetCompletionWaitGroup()->Wait();
        ASSERT_TRUE(evaluate->IsReady());
        Visibility::SurfaceConstants evaluation{};
        evaluation.m_view = viewConstants;
        evaluation.m_visibility = graph->GetSRV(Core::TextureView::Create(visibility.Get()));
        evaluation.m_work.m_pixels = graph->GetSRV(surfaceWork.m_pixels.Get());
        evaluation.m_work.m_waves = graph->GetSRV(surfaceWork.m_waves.Get());
        evaluation.m_work.m_ranges = graph->GetSRV(surfaceWork.m_ranges.Get());
        evaluation.m_g0 = graph->GetUAV(Core::TextureView::Create(planes[0].Get()));
        evaluation.m_g1 = graph->GetUAV(Core::TextureView::Create(planes[1].Get()));
        evaluation.m_g2 = graph->GetUAV(Core::TextureView::Create(planes[2].Get()));
        evaluation.m_g3 = graph->GetUAV(Core::TextureView::Create(planes[3].Get()));
        evaluation.m_motion = graph->GetUAV(motion.Get());
        evaluation.m_work.m_resolution = Vector2UInt(64, 64);
        evaluation.m_work.m_tilesPerRow = 1;
        graph->AddDispatchIndirect("EvaluateRasterSurface",
                                   evaluate,
                                   evaluation,
                                   Core::BufferView(surfaceWork.m_arguments.Get()).Slice(0, 12));
        Constants inspect{};
        inspect.m_view = viewConstants;
        inspect.m_visibility = graph->GetSRV(Core::TextureView::Create(visibility.Get()));
        inspect.m_depth = graph->GetSRV(Core::TextureView::Create(depth.Get()));
        inspect.m_g0 = graph->GetSRV(Core::TextureView::Create(planes[0].Get()));
        inspect.m_g1 = graph->GetSRV(Core::TextureView::Create(planes[1].Get()));
        inspect.m_g2 = graph->GetSRV(Core::TextureView::Create(planes[2].Get()));
        inspect.m_motion = graph->GetSRV(motion.Get());
        inspect.m_words = graph->GetUAV(output.Get());
        graph->AddDispatch("InspectRasterProbe", probe, inspect, Core::ComputeWorkGroupCount(2));
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
        readback->InvalidateMappedRange(0, 144);
        EXPECT_EQ(words[0], (7u << Visibility::kObjectTypeShift) | (instance.m_rowIndex + 1));
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
        EXPECT_EQ(words[13], 0x01f3d988u);
        EXPECT_GT(std::bit_cast<float>(words[14]), 0.9999f);
        EXPECT_EQ(words[15], 0u);
        EXPECT_EQ(words[16], 0u);
        EXPECT_EQ(words[18], 0u);
        EXPECT_EQ(words[19], 0u);
        EXPECT_EQ(words[28], 0u);
        EXPECT_NEAR(std::bit_cast<float>(words[32]), 0.1f, 0.0002f);
        EXPECT_NEAR(std::bit_cast<float>(words[33]), 0.2f, 0.0002f);
        EXPECT_EQ(words[34], 0u);
        EXPECT_EQ(words[35], 0u);
        readback->Unmap();
        graph.Reset();
        uploader.Shutdown();
    }
} // namespace FE::Graphics::RasterProbe
