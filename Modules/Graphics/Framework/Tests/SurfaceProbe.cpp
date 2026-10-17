#include "SurfaceProbe.h"
#include "GpuProbe.h"
#include <Graphics/Core/DescriptorManager.h>
#include <Graphics/Core/GraphicsQueue.h>
#include <Graphics/Core/PipelineFactory.h>
#include <Graphics/Core/PipelineVariantSet.h>
#include <Graphics/Core/ResourcePool.h>
#include <Graphics/Core/RingUploader.h>
#include <Graphics/Core/Vulkan/GraphicsQueue.h>
#include <Graphics/Passes/SurfaceWorkBuilder.h>
#include <Shaders/Passes/MeshPass/GpuWork.h>
#include <gtest/gtest.h>

namespace FE::Graphics::SurfaceProbe
{
    void Check(Core::Device* device, Core::ResourcePool* pool, Core::GraphicsQueue* queue, Core::DescriptorManager* descriptors,
               Core::PipelineFactory* pipelines)
    {
        Core::CompileGlobalPipelineSets(pipelines);
        Core::WaitForGlobalPipelineSets();
        DB::Database database(device, pool);
        MeshInstanceTable instances(&database);
        MeshGroupTable groups(&database);
        MaterialInstanceTable materials(&database);
        constexpr uint32_t kGroupCount = 257;
        const auto unusedGroups = groups.AllocateRows(kGroupCount);
        const auto instanceRows = instances.AllocateRows(kGroupCount);
        const auto groupRows = groups.AllocateRows(kGroupCount);
        EXPECT_NE(groupRows.m_rowIndex, unusedGroups.m_rowIndex);
        EXPECT_GT(groupRows.m_rowIndex, 0u);
        const auto materialRows = materials.AllocateRows(kGroupCount);
        for (uint32_t index = 0; index < kGroupCount; ++index)
        {
            instances.WriteRow(instanceRows.m_rowIndex + index).m_meshGroup.Get() = { groupRows.m_rowIndex + index };
            groups.WriteRow(groupRows.m_rowIndex + index).m_materialInstance.Get() = { materialRows.m_rowIndex + index };
            const auto material = materials.WriteRow(materialRows.m_rowIndex + index);
            material.m_evaluationSlot.Get() = index % 3;
            material.m_shadingSlot.Get() = 0;
        }

        const Vector2UInt kResolution(257, 129);
        constexpr uint32_t kPixelCount = 257 * 129;
        Rc<Core::FrameGraph> graph = device->CreateFrameGraph(descriptors, pool, queue);
        Core::RingUploader uploader;
        uploader.Setup("SurfaceProbeUploader", pool, 256 * 1024);
        const auto classifier = [&](const Env::Name path) {
            Core::ComputePipelineRequest request;
            request.m_desc.SetComputeShader(path);
            Core::ComputePipeline* result = pipelines->CreateComputePipeline(request);
            result->GetCompletionWaitGroup()->Wait();
            return result;
        };
        Core::ComputePipeline* countPipeline = classifier("Shaders/Passes/MeshPass/CountMeshSurfacePixels.cs.hlsl");
        Core::ComputePipeline* scatterPipeline = classifier("Shaders/Passes/MeshPass/ScatterMeshSurfacePixels.cs.hlsl");
        Rc<Core::Buffer> sceneBuffer = Core::Buffer::CreateStructured<MeshPass::MeshSceneData>(device, "ProbeScene", 1);
        Rc<Core::Buffer> viewBuffer = Core::Buffer::CreateStructured<MeshPass::MeshViewData>(device, "SurfaceProbeView", 1);
        Rc<Core::Texture> visibility =
            Core::Texture::Create(device, "SurfaceProbeVisibility", Core::Format::kR32G32_UINT, kResolution);
        Rc<Core::Texture> g0 = Core::Texture::Create(device, "SurfaceProbeG0", Core::Format::kR32_UINT, kResolution);
        Rc<Core::Buffer> marks = Core::Buffer::CreateStructured<uint32_t>(device, "SurfaceProbeMarks", kPixelCount * 2);
        Rc<Core::Buffer> readback = Core::Buffer::CreateByteAddress(device, "SurfaceProbeReadback", kPixelCount * 8);
        Rc<Core::Buffer> argsReadback = Core::Buffer::CreateByteAddress(device, "SurfaceProbeArgsReadback", 3 * 17 * 12);
        Rc<Core::Buffer> codec = Core::Buffer::CreateByteAddress(device, "SurfaceCodecProbe", 32);
        Rc<Core::Buffer> codecReadback = Core::Buffer::CreateByteAddress(device, "SurfaceCodecReadback", 32);
        Core::ResourceCommitParams commit;
        commit.m_memory = Core::ResourceMemory::kHostRandomAccess;
        commit.m_bindFlags = Core::BarrierAccessFlags::kCopyDest;
        pool->CommitBufferMemory(readback.Get(), commit);
        pool->CommitBufferMemory(argsReadback.Get(), commit);
        pool->CommitBufferMemory(codecReadback.Get(), commit);
        Core::ComputePipelineRequest generateRequest;
        generateRequest.m_desc.SetComputeShader("Shaders/Tests/SurfaceProbeGenerate.cs.hlsl");
        Core::ComputePipeline* generate = pipelines->CreateComputePipeline(generateRequest);
        generate->GetCompletionWaitGroup()->Wait();
        ASSERT_TRUE(generate->IsReady());
        Core::ComputePipelineRequest codecRequest;
        codecRequest.m_desc.SetComputeShader("Shaders/Tests/SurfaceCodecProbe.cs.hlsl");
        Core::ComputePipeline* codecPipeline = pipelines->CreateComputePipeline(codecRequest);
        codecPipeline->GetCompletionWaitGroup()->Wait();
        ASSERT_TRUE(codecPipeline->IsReady());

        Visibility::SurfaceWork work;
        for (const uint32_t testCase : { 32u, 64u, 1u, 32u, 0u, 2u, 3u, 4u })
        {
            const bool shadingTest = testCase == 3 || testCase == 4;
            const uint32_t shaderCount = testCase == 1 ? 1 : testCase == 64 || shadingTest ? 17 : 5;
            const uint32_t activeShaders = Math::Min(shaderCount, 3u);
            const uint32_t shaderStride = testCase == 64 || shadingTest ? 8 : 1;
            festd::vector<Visibility::SurfaceBinding> bindings;
            for (uint32_t index = 0; index < kGroupCount; ++index)
            {
                const uint32_t slot = (index % activeShaders) * shaderStride;
                materials.WriteRow(materialRows.m_rowIndex + index).m_evaluationSlot.Get() = slot;
                if (testCase != 2 && !shadingTest)
                    bindings.push_back({ groupRows.m_rowIndex + index, slot });
            }

            if (shadingTest)
            {
                for (uint32_t slot = 0; slot < shaderCount; ++slot)
                    bindings.push_back({ slot, slot });
            }

            const uint32_t laneCount = testCase == 64 || testCase == 4 ? 64 : 32;
            work = Visibility::CreateSurfaceWork(device,
                                                 kResolution,
                                                 bindings,
                                                 groups.GetReservedRowCount(),
                                                 shaderCount,
                                                 laneCount);
            EXPECT_EQ(work.m_tileCount, 6u);
            EXPECT_EQ(work.m_bucketCount, testCase == 2 ? 0u : 6u * (shadingTest ? shaderCount : kGroupCount));
            if (testCase == 2)
                EXPECT_EQ(work.m_waveCapacity, 0u);
            Core::ComputePipelineRequest visitRequest;
            visitRequest.m_desc.SetComputeShader("Shaders/Tests/SurfaceProbeVisit.cs.hlsl");
            visitRequest.m_defines =
                Env::Name(Fmt::Format("VISIT_LANES={} VISIT_SHADER_COUNT={} VISIT_SHADER_STRIDE={} VISIT_SHADING={}",
                                      laneCount,
                                      activeShaders,
                                      shaderStride,
                                      shadingTest ? 1 : 0));
            Core::ComputePipeline* visit = pipelines->CreateComputePipeline(visitRequest);
            visit->GetCompletionWaitGroup()->Wait();
            ASSERT_TRUE(visit->IsReady());
            generateRequest.m_defines = shadingTest ? Env::Name("GENERATE_SHADING=1") : Env::Name::kEmpty;
            generate = pipelines->CreateComputePipeline(generateRequest);
            generate->GetCompletionWaitGroup()->Wait();
            ASSERT_TRUE(generate->IsReady());
            queue->BeginFrame();
            graph->BeginFrame();
            database.Update(*graph, queue->GetCurrentFence());
            MeshPass::MeshViewData view{};
            MeshPass::MeshSceneData scene{};
            scene.m_instances = instances.GetDeviceAddress();
            scene.m_groups = groups.GetDeviceAddress();
            scene.m_materials = materials.GetDeviceAddress();
            view.m_scene = graph->GetSRV(sceneBuffer.Get());
            FE_Verify(uploader.Upload(*graph, sceneBuffer.Get(), scene));
            auto* sceneAccess = graph->AllocatePassData<Core::BufferAccessPassDesc>();
            sceneAccess->m_access = { sceneBuffer.Get(),
                                      Core::BarrierSyncFlags::kAllShading,
                                      Core::BarrierAccessFlags::kShaderRead };
            graph->AddPass("ProbeSceneReady", sceneAccess);
            FE_Verify(uploader.Upload(*graph, viewBuffer.Get(), view));
            auto* seed = graph->AllocatePassData<ProbePassDesc>();
            seed->m_pipeline = generate;
            seed->m_constants.m_resolution = kResolution;
            seed->m_constants.m_visibility = graph->GetUAV(Core::TextureView::Create(visibility.Get()));
            seed->m_constants.m_g0 = graph->GetUAV(Core::TextureView::Create(g0.Get()));
            seed->m_constants.m_marks = graph->GetUAV(marks.Get());
            seed->m_constants.m_instanceBase = instanceRows.m_rowIndex;
            seed->m_constants.m_groupCount = testCase == 0 || testCase == 2 ? 0 : kGroupCount;
            graph->AddDispatchPass("GenerateSurfaceProbe", seed, Math::CeilDivide(kResolution, 8u));
            Visibility::BuildSurfaceWork(*graph,
                                         uploader,
                                         shadingTest ? nullptr : viewBuffer.Get(),
                                         shadingTest ? nullptr : visibility.Get(),
                                         g0.Get(),
                                         work,
                                         shadingTest,
                                         countPipeline,
                                         scatterPipeline);
            for (uint32_t slot = 0; slot < work.m_shaderCount; ++slot)
            {
                auto* evaluation = graph->AllocatePassData<ProbeIndirectDesc>();
                evaluation->m_pipeline = visit;
                evaluation->m_arguments = Core::BufferView(work.m_arguments.Get()).Slice(slot * 12, 12);
                evaluation->m_constants.m_resolution = kResolution;
                evaluation->m_constants.m_groupBase = groupRows.m_rowIndex;
                evaluation->m_constants.m_groupCount = kGroupCount;
                evaluation->m_constants.m_shaderSlot = slot;
                evaluation->m_constants.m_pixels = graph->GetSRV(work.m_pixels.Get());
                evaluation->m_constants.m_waves = graph->GetSRV(work.m_waves.Get());
                evaluation->m_constants.m_ranges = graph->GetSRV(work.m_ranges.Get());
                evaluation->m_constants.m_marks = graph->GetUAV(marks.Get());
                graph->AddPass("VisitSurfaceProbe", evaluation, [](Core::FrameGraphContext& context) {
                    context.DispatchIndirect();
                });
            }
            graph->AddCopyPass(Core::BufferView(argsReadback.Get()).Slice(0, shaderCount * 12),
                               Core::BufferView(work.m_arguments.Get()).Slice(0, shaderCount * 12));

            auto* pack = graph->AllocatePassData<Stage1Probe::PassDesc>();
            pack->m_pipeline = codecPipeline;
            pack->m_constants.m_words = graph->GetUAV(codec.Get());
            graph->AddDispatchPass("SurfaceCodecProbe", pack, Core::ComputeWorkGroupCount(1));
            graph->AddCopyPass(Core::BufferView(codecReadback.Get()), Core::BufferView(codec.Get()));
            graph->AddCopyPass(Core::BufferView(readback.Get()), Core::BufferView(marks.Get()));
            graph->CompileAndExecute();
            uploader.CloseFrame(queue->GetCurrentFence());
            Vulkan::CommandBuffer* native = Vulkan::ImplCast(queue)->GetCurrentCommandBuffer();
            const Core::FenceSyncPoint fence = queue->CloseFrame();
            native->EnqueueFenceToSignal(fence);
            native->Submit();
            fence.Wait();
            device->WaitIdle();
            const uint32_t* results = static_cast<const uint32_t*>(readback->Map());
            readback->InvalidateMappedRange(0, kPixelCount * 8);
            uint32_t wrong = 0;
            for (uint32_t pixel = 0; pixel < kPixelCount; ++pixel)
                wrong += results[pixel * 2]
                        != (testCase == 0 || testCase == 2 || pixel % 7 == 0 || (shadingTest && pixel % 11 == 0) ? 0u : 1u)
                    || results[pixel * 2 + 1] != 0;

            EXPECT_EQ(wrong, 0u);
            readback->Unmap();
            const uint32_t* args = static_cast<const uint32_t*>(argsReadback->Map());
            argsReadback->InvalidateMappedRange(0, shaderCount * 12);
            for (uint32_t slot = 0; slot < work.m_shaderCount; ++slot)
            {
                const bool active =
                    testCase != 0 && testCase != 2 && slot % shaderStride == 0 && slot / shaderStride < activeShaders;
                if (active)
                    EXPECT_GT(args[slot * 3], 0u);
                else
                    EXPECT_EQ(args[slot * 3], 0u);
            }

            argsReadback->Unmap();
            const uint32_t* encoded = static_cast<const uint32_t*>(codecReadback->Map());
            codecReadback->InvalidateMappedRange(0, 32);
            EXPECT_EQ(encoded[0], 19u);
            EXPECT_LT(std::bit_cast<float>(encoded[1]), 0.004f);
            EXPECT_GT(std::bit_cast<float>(encoded[2]), 0.99999f);
            EXPECT_NEAR(std::bit_cast<float>(encoded[3]), 1000.0f, 8.0f);
            EXPECT_EQ(std::bit_cast<float>(encoded[4]), 32.0f);
            EXPECT_EQ(std::bit_cast<float>(encoded[5]), 0.0f);
            EXPECT_EQ(encoded[6], 0x0100ff80u);
            EXPECT_EQ(encoded[7], 0u);
            codecReadback->Unmap();
        }

        graph.Reset();
        uploader.Shutdown();
    }
} // namespace FE::Graphics::SurfaceProbe
