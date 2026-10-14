#include "RasterProbe.h"
#include <Core/Math/Packing.h>
#include <Graphics/Core/DescriptorManager.h>
#include <Graphics/Core/GraphicsQueue.h>
#include <Graphics/Core/PipelineFactory.h>
#include <Graphics/Core/ResourcePool.h>
#include <Graphics/Core/RingUploader.h>
#include <Graphics/Core/Vulkan/GraphicsQueue.h>
#include <Graphics/Passes/VisibilityPass.h>
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
               Core::PipelineFactory* pipelines)
    {
        DB::Database database(device, pool);
        MeshInstanceTable instances(&database);
        MeshGroupTable groups(&database);
        MeshLodInfoTable lods(&database);
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
        Rc<Core::Buffer> output = Core::Buffer::CreateByteAddress(device, "RasterProbeOutput", 96);
        Rc<Core::Buffer> readback = Core::Buffer::CreateByteAddress(device, "RasterProbeReadback", 96);
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
        auto* inspect = graph->AllocatePassData<PassDesc>();
        inspect->m_pipeline = probe;
        inspect->m_constants.m_view = graph->GetSRV(viewBuffer.Get());
        inspect->m_constants.m_visibility = graph->GetSRV(Core::TextureView::Create(visibility.Get()));
        inspect->m_constants.m_depth = graph->GetSRV(Core::TextureView::Create(depth.Get()));
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
        readback->InvalidateMappedRange(0, 96);
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
        EXPECT_EQ(words[12], 0u);
        EXPECT_EQ(words[14], 0u);
        EXPECT_EQ(words[15], 0u);
        readback->Unmap();
        graph.Reset();
        uploader.Shutdown();
    }
} // namespace FE::Graphics::RasterProbe
