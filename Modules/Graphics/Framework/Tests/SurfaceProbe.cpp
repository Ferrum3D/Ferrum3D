#include "SurfaceProbe.h"
#include "GpuProbe.h"
#include <Core/CLI/CommandLine.h>
#include <Graphics/Core/DescriptorManager.h>
#include <Graphics/Core/GraphicsQueue.h>
#include <Graphics/Core/PipelineFactory.h>
#include <Graphics/Core/PipelineVariantSet.h>
#include <Graphics/Core/ResourcePool.h>
#include <Graphics/Core/RingUploader.h>
#include <Graphics/Core/Vulkan/Device.h>
#include <Graphics/Core/Vulkan/GraphicsQueue.h>
#include <Graphics/Passes/SurfaceWorkBuilder.h>
#include <Shaders/Passes/MeshPass/GpuWork.h>
#include <gtest/gtest.h>

namespace FE::Graphics::SurfaceProbe
{
    namespace
    {
        constexpr uint32_t kEmissionSamples = 16;
        constexpr uint32_t kEmissionBatchSize = 16;


        // Opt-in timing stays in the probe; production uses only the indirect path.
        void BenchmarkEmission(Core::FrameGraph& graph, const Visibility::SurfaceWork& work, Core::ComputePipeline* pipeline,
                               Core::GraphicsQueue* queue, VkQueryPool queries)
        {
            Visibility::SurfaceCommandsConstants constants{};
            constants.m_offsets = graph.GetSRV(work.m_scanLevels.front().m_offsets.Get());
            constants.m_totals = graph.GetSRV(work.m_scanLevels.back().m_sums.Get());
            constants.m_countsRead = graph.GetSRV(work.m_counts.Get());
            constants.m_bindings = graph.GetSRV(work.m_routing->m_bindings.Get());
            constants.m_shaderBindings = graph.GetSRV(work.m_routing->m_shaderBindings.Get());
            constants.m_waves = graph.GetUAV(work.m_waves.Get());
            constants.m_ranges = graph.GetUAV(work.m_ranges.Get());
            constants.m_arguments = graph.GetUAV(work.m_arguments.Get());
            constants.m_tileCount = work.m_tileCount;
            constants.m_bucketCount = work.m_bucketCount;
            constants.m_shaderCount = work.m_shaderCount;
            constants.m_waveLaneCount = work.m_waveLaneCount;
            const VkCommandBuffer commands = Vulkan::ImplCast(queue)->GetCurrentCommandBuffer()->GetNative();
            graph.AddPassWithoutBarriers("ResetEmissionTimings", [=](Core::FrameGraphContext&) {
                vkCmdResetQueryPool(commands, queries, 0, kEmissionSamples * 4);
            });

            for (uint32_t sample = 0; sample < kEmissionSamples; ++sample)
            {
                for (uint32_t variant = 0; variant < 2; ++variant)
                {
                    const bool indirect = (variant + sample) % 2 != 0;
                    const uint32_t query = sample * 4 + (indirect ? 2 : 0);
                    graph.AddPassWithoutBarriers("BeginEmissionTiming", [=](Core::FrameGraphContext&) {
                        vkCmdWriteTimestamp2(commands, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, queries, query);
                    });
                    for (uint32_t repeat = 0; repeat < kEmissionBatchSize; ++repeat)
                    {
                        if (indirect)
                        {
                            auto* pass = graph.AllocatePassData<Visibility::SurfaceCommandsPassDesc>();
                            pass->m_constants = constants;
                            pass->m_pipeline = pipeline;
                            pass->m_arguments = Core::BufferView(work.m_emissionArguments.Get());
                            graph.AddDispatchIndirectPass("BenchmarkIndirectEmission", pass);
                        }
                        else
                        {
                            auto* pass = graph.AllocatePassData<EmissionDirectPassDesc>();
                            pass->m_constants = constants;
                            pass->m_pipeline = pipeline;
                            const uint32_t threads = Math::Max(work.m_waveCapacity, work.m_shaderCount);
                            const uint32_t groups = Math::Max(Math::CeilDivide(threads, Visibility::kSurfaceBlockSize), 1u);
                            graph.AddDispatchPass("BenchmarkDirectEmission", pass, groups);
                        }
                    }

                    graph.AddPassWithoutBarriers("EndEmissionTiming", [=](Core::FrameGraphContext&) {
                        vkCmdWriteTimestamp2(commands, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, queries, query + 1);
                    });
                }
            }
        }


        void PrintEmissionTimings(Core::Device* device, VkQueryPool queries, const Visibility::SurfaceWork& work,
                                  const uint32_t testCase)
        {
            uint64_t timestamps[kEmissionSamples * 4];
            const VkDevice native = Vulkan::ImplCast(device)->GetNative();
            const VkResult result = vkGetQueryPoolResults(native,
                                                          queries,
                                                          0,
                                                          kEmissionSamples * 4,
                                                          sizeof(timestamps),
                                                          timestamps,
                                                          sizeof(uint64_t),
                                                          VK_QUERY_RESULT_64_BIT);
            ASSERT_EQ(result, VK_SUCCESS);
            const float period = Vulkan::ImplCast(device)->GetAdapterProperties().limits.timestampPeriod;
            float direct[kEmissionSamples];
            float indirect[kEmissionSamples];
            for (uint32_t index = 0; index < kEmissionSamples; ++index)
            {
                direct[index] = (timestamps[index * 4 + 1] - timestamps[index * 4]) * period / (1000.0f * kEmissionBatchSize);
                indirect[index] =
                    (timestamps[index * 4 + 3] - timestamps[index * 4 + 2]) * period / (1000.0f * kEmissionBatchSize);
            }

            festd::sort(festd::begin(direct), festd::end(direct));
            festd::sort(festd::begin(indirect), festd::end(indirect));
            Logger::LogInfo("Emission benchmark {}x{}, case {}, {} bindings: direct {} us, indirect {} us (median, {} samples)",
                            work.m_resolution.x,
                            work.m_resolution.y,
                            testCase,
                            work.m_routing->m_bindingCount,
                            direct[kEmissionSamples / 2],
                            indirect[kEmissionSamples / 2],
                            kEmissionSamples);
        }
    } // namespace


    void Check(Core::Device* device, Core::ResourcePool* pool, Core::GraphicsQueue* queue, Core::DescriptorManager* descriptors,
               Core::PipelineFactory* pipelines, const bool largeDispatch)
    {
        Core::CompileGlobalPipelineSets(pipelines);
        Core::WaitForGlobalPipelineSets();
        DB::Database database(device, pool);
        MeshInstanceTable instances(&database);
        MeshGroupTable groups(&database);
        MaterialInstanceTable materials(&database);
        const bool benchmark = Cli::Check("--benchmark-surface-emission");
        const uint32_t maxGroupCount = Math::Min(MeshGroupTable::kRowsPerPage, MeshInstanceTable::kRowsPerPage);
        const uint32_t kGroupCount = benchmark ? Math::FloorPowerOfTwo(maxGroupCount) : 257;
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

        const Vector2UInt kResolution = largeDispatch ? Vector2UInt(2049, 1281) : Vector2UInt(257, 129);
        const uint32_t kPixelCount = kResolution.x * kResolution.y;
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

        VkQueryPool queries = VK_NULL_HANDLE;
        Core::ComputePipeline* emissionPipeline = nullptr;
        const VkDevice nativeDevice = Vulkan::ImplCast(device)->GetNative();
        if (benchmark)
        {
            VkQueryPoolCreateInfo info{ .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };
            info.queryType = VK_QUERY_TYPE_TIMESTAMP;
            info.queryCount = kEmissionSamples * 4;
            ASSERT_EQ(vkCreateQueryPool(nativeDevice, &info, nullptr, &queries), VK_SUCCESS);
            emissionPipeline = classifier("Shaders/Deferred/EmitSurfaceCommands.cs.hlsl");
        }

        const auto destroyQueries = festd::defer([&] {
            if (queries)
                vkDestroyQueryPool(nativeDevice, queries, nullptr);
        });

        Visibility::SurfaceWork work;
        for (const uint32_t testCase : { 32u, 64u, 1u, 32u, 0u, 2u, 3u, 4u, 5u })
        {
            if (testCase == 5 && !benchmark)
                continue;

            if (largeDispatch && testCase != 1 && testCase != 5)
                continue;

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
            auto routing = Visibility::CreateSurfaceRouting(device, bindings, groups.GetReservedRowCount(), shaderCount);
            work = Visibility::CreateSurfaceWork(device, kResolution, routing, laneCount);
            const Vector2UInt tiles = Math::CeilDivide(kResolution, Visibility::kTileSize);
            EXPECT_EQ(work.m_tileCount, tiles.x * tiles.y);
            EXPECT_EQ(work.m_bucketCount, testCase == 2 ? 0u : work.m_tileCount * (shadingTest ? shaderCount : kGroupCount));
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
            generateRequest.m_defines = testCase == 5 ? Env::Name("GENERATE_SPARSE=1")
                : shadingTest                         ? Env::Name("GENERATE_SHADING=1")
                                                      : Env::Name::kEmpty;
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
            graph->AddBufferDependency(viewBuffer.Get(), sceneBuffer.Get());
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
            if (shadingTest)
            {
                Visibility::BuildSurfaceWork(*graph, uploader, Visibility::ShadingWorkInput{ g0.Get() }, work);
            }
            else
            {
                const Visibility::EvaluationWorkInput input{ viewBuffer.Get(), visibility.Get(), countPipeline, scatterPipeline };
                Visibility::BuildSurfaceWork(*graph, uploader, input, work);
            }

            if (benchmark)
                BenchmarkEmission(*graph, work, emissionPipeline, queue, queries);

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
            graph->AddCopyPass(Core::BufferView(argsReadback.Get()).Slice(shaderCount * 12, 12),
                               Core::BufferView(work.m_emissionArguments.Get()));

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
            if (benchmark)
                PrintEmissionTimings(device, queries, work, testCase);

            const uint32_t* results = static_cast<const uint32_t*>(readback->Map());
            readback->InvalidateMappedRange(0, kPixelCount * 8);
            uint32_t wrong = 0;
            for (uint32_t pixel = 0; pixel < kPixelCount; ++pixel)
                wrong += results[pixel * 2]
                        != (testCase == 0 || testCase == 2 || pixel % 7 == 0 || (testCase == 5 && pixel % 101 != 0)
                                    || (shadingTest && pixel % 11 == 0)
                                ? 0u
                                : 1u)
                    || results[pixel * 2 + 1] != 0;

            EXPECT_EQ(wrong, 0u);
            readback->Unmap();
            const uint32_t* args = static_cast<const uint32_t*>(argsReadback->Map());
            argsReadback->InvalidateMappedRange(0, shaderCount * 12 + 12);
            uint32_t totalWaves = 0;
            for (uint32_t slot = 0; slot < work.m_shaderCount; ++slot)
            {
                totalWaves += args[slot * 3];
                const bool active =
                    testCase != 0 && testCase != 2 && slot % shaderStride == 0 && slot / shaderStride < activeShaders;
                if (active)
                    EXPECT_GT(args[slot * 3], 0u);
                else
                    EXPECT_EQ(args[slot * 3], 0u);
            }

            const uint32_t emissionThreads = Math::Max(totalWaves, shaderCount);
            EXPECT_EQ(args[shaderCount * 3], Math::CeilDivide(emissionThreads, Visibility::kSurfaceBlockSize));
            EXPECT_EQ(args[shaderCount * 3 + 1], 1u);
            EXPECT_EQ(args[shaderCount * 3 + 2], 1u);

            if (largeDispatch && testCase != 5)
            {
                EXPECT_GT(args[0], 65535u);
                EXPECT_EQ(args[1], 1u);
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
