#include "GpuProbe.h"
#include <Graphics/Core/AsyncCopyQueue.h>
#include <Graphics/Core/DescriptorManager.h>
#include <Graphics/Core/DeviceFactory.h>
#include <Graphics/Core/GraphicsQueue.h>
#include <Graphics/Core/PipelineFactory.h>
#include <Graphics/Core/ResourcePool.h>
#include <Graphics/Core/Vulkan/GraphicsQueue.h>
#include <Graphics/Materials/MaterialInstance.h>
#include <Graphics/Passes/VisibilitySupport.h>
#include <Graphics/Scene/ComputePassRegistry.h>
#include <gtest/gtest.h>

using namespace FE;
using namespace FE::Graphics;

TEST(VisibilityGpu, ComputeWrittenIndirectArgumentsAndRegistryDeduplication)
{
    Core::DeviceFactory::Init(Core::GraphicsApi::kVulkan);
    const auto shutdown = festd::defer([] {
        Core::DeviceFactory::Shutdown();
    });
    {
        Core::DeviceFactory& factory = Core::DeviceFactory::Get();
        const auto adapters = factory.EnumerateAdapters();
        ASSERT_FALSE(adapters.empty());
        Rc<Core::Device> device = factory.CreateDevice(adapters.front().m_name);
        Rc<Core::GraphicsQueue> queue = device->CreateGraphicsQueue();
        Rc<Core::AsyncCopyQueue> copyQueue = device->CreateAsyncCopyQueue();
        Rc<Core::ResourcePool> pool = device->CreateResourcePool(queue.Get(), copyQueue.Get());
        {
            Rc<Core::DescriptorManager> descriptors = device->CreateDescriptorManager();
            Rc<Core::FrameGraph> graph = device->CreateFrameGraph(descriptors.Get(), pool.Get(), queue.Get());
            Rc<Core::PipelineFactory> pipelines = device->CreatePipelineFactory(descriptors.Get());

            Core::ComputePipelineRequest request;
            request.m_desc.SetComputeShader(Env::Name("Shaders/Tests/GpuProbe.cs.hlsl"));
            Core::ComputePipeline* pipeline = pipelines->CreateComputePipeline(request);
            pipeline->GetCompletionWaitGroup()->Wait();
            ASSERT_TRUE(pipeline->IsReady());
            ComputePassRegistry evaluationPipelines;
            ComputePassRegistry shadingPipelines;
            EXPECT_EQ(evaluationPipelines.Register(nullptr), kInvalidIndex);
            EXPECT_EQ(evaluationPipelines.Register(pipeline), 0u);
            EXPECT_EQ(evaluationPipelines.Register(pipeline), 0u);

            Core::ComputePipelineRequest shadingRequest = request;
            shadingRequest.m_defines = Env::Name("SHADE_PASS=1");
            Core::ComputePipeline* shadingPipeline = pipelines->CreateComputePipeline(shadingRequest);
            shadingPipeline->GetCompletionWaitGroup()->Wait();
            ASSERT_TRUE(shadingPipeline->IsReady());
            EXPECT_NE(shadingPipeline, pipeline);
            EXPECT_EQ(shadingPipelines.Register(shadingPipeline), 0u);
            EXPECT_EQ(shadingPipelines.Register(shadingPipeline), 0u);
            EXPECT_EQ(shadingPipelines.Register(pipeline), 1u);
            EXPECT_EQ(evaluationPipelines.GetPipelines().size(), 1u);
            EXPECT_EQ(shadingPipelines.GetPipelines().size(), 2u);

            const VisibilitySupport support = VisibilitySupport::Query(*device);
            EXPECT_TRUE(support.m_supported);
            EXPECT_TRUE(support.m_hdrBytesPerPixel == 4 || support.m_hdrBytesPerPixel == 8);

            if (support.m_waveLaneCount != 0)
            {
                Core::ComputePipelineRequest waveRequest = request;
                waveRequest.m_desc.m_waveLaneCount = support.m_waveLaneCount;
                Core::ComputePipeline* wavePipeline = pipelines->CreateComputePipeline(waveRequest);
                wavePipeline->GetCompletionWaitGroup()->Wait();
                EXPECT_TRUE(wavePipeline->IsReady());
                EXPECT_NE(wavePipeline, pipeline);
            }

            MaterialAsset material;
            material.m_name = "Stage1Probe";
            for (const char* role : { "SurfaceEvaluate", "DeferredShade" })
            {
                MaterialTechniqueDesc& technique = material.m_techniques.emplace_back();
                technique.m_role = Env::Name(role);
                technique.m_kind = MaterialTechniqueKind::kCompute;
                technique.m_computeShader = request.m_desc.m_shader;
                if (technique.m_role == Env::Name("DeferredShade"))
                    technique.m_shaderDefines = "SHADE_PASS=1";
            }

            MaterialParameterDesc& tint = material.m_parameters.emplace_back();
            tint.m_name = Env::Name("Tint");
            tint.m_type = MaterialParameterType::kVector;
            tint.m_defaultValue = Vector4(0.25f, 0.5f, 0.75f, 1.0f);
            MaterialParameterDesc& scale = material.m_parameters.emplace_back();
            scale.m_name = Env::Name("UvScale");
            scale.m_type = MaterialParameterType::kVector;
            scale.m_scope = MaterialParameterScope::kInstance;
            scale.m_defaultValue = Vector4(2.0f, 3.0f, 0.0f, 0.0f);
            MaterialInstanceAsset instance;
            MaterialParameterAllocator parameters(device.Get(), pool.Get());
            MaterialInstanceRuntime runtime(&material, &instance, &parameters, pipelines.Get());
            const MaterialParameterAllocator::Allocation instanceParameters = runtime.AllocateInstanceParameters();
            EXPECT_EQ(runtime.GetComputePipeline(Env::Name("SurfaceEvaluate")), pipeline);
            EXPECT_EQ(runtime.GetComputePipeline(Env::Name("DeferredShade")), shadingPipeline);
            EXPECT_EQ(runtime.GetComputePipeline(Env::Name("MissingRole")), nullptr);

            Rc<Core::Buffer> arguments = Core::Buffer::CreateByteAddress(device.Get(), "IndirectProbeArguments", 32);
            Rc<Core::Buffer> output = Core::Buffer::CreateByteAddress(device.Get(), "IndirectProbeOutput", 12);
            Rc<Core::Buffer> readback = Core::Buffer::CreateByteAddress(device.Get(), "IndirectProbeReadback", 12);
            Core::ResourceCommitParams commit;
            commit.m_memory = Core::ResourceMemory::kHostRandomAccess;
            commit.m_bindFlags = Core::BarrierAccessFlags::kCopyDest;
            pool->CommitBufferMemory(readback.Get(), commit);

            queue->BeginFrame();
            graph->BeginFrame();
            const Core::FenceSyncPoint completion = queue->GetCurrentFence();
            parameters.Update(*graph, completion);
            auto* write = graph->AllocatePassData<Stage1Probe::PassDesc>();
            write->m_pipeline = pipeline;
            write->m_constants.m_words = graph->GetUAV(Core::BufferView(arguments.Get()).Slice(16, 12));
            write->m_constants.m_writeArguments = 1;
            graph->AddDispatchPass("WriteIndirectArguments", write, Core::ComputeWorkGroupCount(1));

            auto* consume = graph->AllocatePassData<Stage1Probe::IndirectPassDesc>();
            consume->m_pipeline = pipeline;
            consume->m_constants.m_words = graph->GetUAV(output.Get());
            consume->m_constants.m_writeArguments = 2;
            consume->m_constants.m_material = runtime.GetMaterialParameters();
            consume->m_constants.m_instance = instanceParameters.m_devicePointer;
            consume->m_arguments = Core::BufferView(arguments.Get()).Slice(12, 20);
            graph->AddPass("ConsumeIndirectArguments", consume, [](Core::FrameGraphContext& context) {
                context.DispatchIndirect(4);
            });
            graph->AddCopyPass(readback.Get(), output.Get());
            graph->CompileAndExecute();
            Vulkan::CommandBuffer* commandBuffer = Vulkan::ImplCast(queue.Get())->GetCurrentCommandBuffer();
            const Core::FenceSyncPoint submitted = queue->CloseFrame();
            commandBuffer->EnqueueFenceToSignal(submitted);
            commandBuffer->Submit();
            submitted.Wait();
            device->WaitIdle();

            const uint32_t* words = static_cast<const uint32_t*>(readback->Map());
            readback->InvalidateMappedRange(0, 12);
            EXPECT_EQ(words[0], std::bit_cast<uint32_t>(2.25f));
            EXPECT_EQ(words[1], std::bit_cast<uint32_t>(2.25f));
            EXPECT_EQ(words[2], std::bit_cast<uint32_t>(2.25f));
            readback->Unmap();
            graph.Reset();
            copyQueue->Drain();
            device->WaitIdle();
        }
        device->WaitIdle();
        queue.Reset();
        device->WaitIdle();
        copyQueue.Reset();
        device->WaitIdle();
    }
}
