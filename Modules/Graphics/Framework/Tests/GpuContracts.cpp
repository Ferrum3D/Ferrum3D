#include "DeferredProbe.h"
#include "GpuProbe.h"
#include "RasterProbe.h"
#include "SurfaceProbe.h"
#include <Core/Base/PlatformInclude.h>
#include <Core/Jobs/Jobs.h>
#include <Core/Threading/Thread.h>
#include <Graphics/Core/AsyncCopyQueue.h>
#include <Graphics/Core/DescriptorManager.h>
#include <Graphics/Core/DeviceFactory.h>
#include <Graphics/Core/GraphicsQueue.h>
#include <Graphics/Core/PipelineFactory.h>
#include <Graphics/Core/ResourcePool.h>
#include <Graphics/Core/Viewport.h>
#include <Graphics/Core/Vulkan/Device.h>
#include <Graphics/Core/Vulkan/DeviceFactory.h>
#include <Graphics/Core/Vulkan/GraphicsQueue.h>
#include <Graphics/Core/Vulkan/Platform/VulkanSurface.h>
#include <Graphics/Materials/MaterialInstance.h>
#include <Graphics/Passes/VisibilityPass.h>
#include <Graphics/Renderer.h>
#include <Graphics/Scene/ComputePassRegistry.h>
#include <Graphics/Scene/Scene.h>
#include <Graphics/Scene/View.h>
#include <gtest/gtest.h>

using namespace FE;
using namespace FE::Graphics;

namespace
{
    void CheckSplitAsyncUpload(Core::Device* device, Core::ResourcePool* pool, Core::AsyncCopyQueue* copyQueue)
    {
        constexpr uint32_t kByteSize = 16 * 1024 * 1024 + 4096;
        festd::vector<std::byte> source(kByteSize, std::byte{ 0x5a });
        const Rc destination = Core::Buffer::CreateByteAddress(device, "SplitUploadProbe", kByteSize);
        const Core::ResourceCommitParams commit{ .m_bindFlags = Core::BarrierAccessFlags::kCopyDest,
                                                 .m_memory = Core::ResourceMemory::kHostRandomAccess,
                                                 .m_queue = Core::DeviceQueueType::kTransfer };
        pool->CommitBufferMemory(destination.Get(), commit);

        Memory::LinearAllocator allocator;
        Core::AsyncCopyCommandListBuilder builder(&allocator, 4096);
        builder.UploadBuffer(destination.Get(), source.data());
        const Rc<WaitGroup> done = WaitGroup::Create();
        copyQueue->ExecuteCommandList(builder.Build(&allocator, done.Get()));
        done->Wait();
        copyQueue->Drain();

        const auto* bytes = static_cast<const std::byte*>(destination->Map());
        destination->InvalidateMappedRange(0, kByteSize);
        EXPECT_EQ(bytes[0], std::byte{ 0x5a });
        EXPECT_EQ(bytes[16 * 1024 * 1024 - 1], std::byte{ 0x5a });
        EXPECT_EQ(bytes[16 * 1024 * 1024], std::byte{ 0x5a });
        EXPECT_EQ(bytes[kByteSize - 1], std::byte{ 0x5a });
        destination->Unmap();
    }


    void CheckDescriptorSnapshots(Core::Device* device, Core::ResourcePool* pool, Core::GraphicsQueue* queue,
                                  Core::DescriptorManager* descriptors, Core::FrameGraph* graph, Core::ComputePipeline* pipeline)
    {
        const Rc first = Core::Buffer::CreateByteAddress(device, "DescriptorSnapshotFirst", 12);
        const Rc second = Core::Buffer::CreateByteAddress(device, "DescriptorSnapshotSecond", 12);
        const Core::ResourceCommitParams commit{ .m_bindFlags = Core::BarrierAccessFlags::kShaderRead
                                                     | Core::BarrierAccessFlags::kShaderWrite,
                                                 .m_memory = Core::ResourceMemory::kHostRandomAccess };
        pool->CommitBufferMemory(first.Get(), commit);
        pool->CommitBufferMemory(second.Get(), commit);
        const BufferUAVDescriptor descriptor = descriptors->CreateUAV(first.Get());
        for (uint32_t frame = 0; frame < 6; ++frame)
        {
            uint32_t* firstWord = static_cast<uint32_t*>(first->Map());
            uint32_t* secondWord = static_cast<uint32_t*>(second->Map());
            *firstWord = 0;
            *secondWord = 0;
            first->FlushMappedRange(0, 12);
            second->FlushMappedRange(0, 12);

            queue->BeginFrame();
            graph->BeginFrame();
            auto* pass = graph->AllocatePassData<Stage1Probe::PassDesc>();
            pass->m_pipeline = pipeline;
            pass->m_constants.m_words = descriptor;
            pass->m_constants.m_writeArguments = 0;
            graph->AddDispatchPass("FrozenDescriptorProbe", pass, Core::ComputeWorkGroupCount(1));
            descriptors->Update(descriptor, frame % 2 == 0 ? second.Get() : first.Get());
            graph->CompileAndExecute();
            const Core::FenceSyncPoint completion = queue->GetCurrentFence();
            queue->SubmitFrame();
            completion.Wait();
            first->InvalidateMappedRange(0, 12);
            second->InvalidateMappedRange(0, 12);
            EXPECT_EQ(*firstWord, frame % 2 == 0 ? 100u : 0u);
            EXPECT_EQ(*secondWord, frame % 2 == 0 ? 0u : 100u);
            first->Unmap();
            second->Unmap();
        }

        descriptors->FreePersistentDescriptor(descriptor.m_value);
    }


    struct SuspendedRenderViewport final : Core::Viewport
    {
        Rc<WaitGroup> m_acquiring = WaitGroup::Create();
        Rc<WaitGroup> m_resume = WaitGroup::Create();
        Core::FenceSyncPoint m_submissionWait;
        bool m_acquiredOnMainThread = true;
        uint64_t m_acquireWorker = 0;
        Core::ViewportDesc m_desc{ 64, 64, 0 };

        explicit SuspendedRenderViewport(Core::Device* device)
        {
            m_device = device;
            SetImmediateDestroyPolicy();
        }


        void Init(const Core::ViewportDesc& desc) override
        {
            m_desc = desc;
        }


        const Core::ViewportDesc& GetDesc() const override
        {
            return m_desc;
        }


        Core::Texture* GetCurrentColorTarget() override
        {
            return nullptr;
        }


        bool AcquireNextImage() override
        {
            m_acquiredOnMainThread = Threading::IsMainThread();
            m_acquireWorker = festd::to_underlying(Jobs::GetAffinityMaskForCurrentThread());
            m_acquiring->Signal();
            m_resume->Wait();
            Vulkan::ImplCast(Renderer::Get().GetGraphicsQueue())->GetCurrentCommandBuffer()->EnqueueFenceToWait(m_submissionWait);
            return false;
        }


        void PrepareBlit() override {}

    private:
        void DestroyObject() override
        {
            Memory::DefaultDelete(this);
        }
    };
} // namespace


TEST(AsyncRenderer, SceneHandoffPrecedesAcquisitionAndSuspendedFramesRetire)
{
    Core::DeviceFactory::Init(Core::GraphicsApi::kVulkan);
    const auto shutdown = festd::defer([] {
        Core::DeviceFactory::Shutdown();
    });
    {
        const auto adapters = Core::DeviceFactory::Get().EnumerateAdapters();
        ASSERT_FALSE(adapters.empty());
        const Rc device = Core::DeviceFactory::Get().CreateDevice(adapters.front().m_name);
        Renderer::Init(device.Get());
        auto& renderer = Renderer::Get();
        const Rc viewport = Memory::DefaultNew<SuspendedRenderViewport>(device.Get());
        const Rc gpuHold = device->CreateFence(0);
        viewport->m_submissionWait = { gpuHold, 1 };
        Scene* scene = renderer.CreateScene();
        Core::DescriptorManager* descriptors = renderer.GetDescriptorManager();
        Rc first = Core::Buffer::CreateByteAddress(device.Get(), "AsyncDescriptorFirst", 16);
        Rc second = Core::Buffer::CreateByteAddress(device.Get(), "AsyncDescriptorSecond", 16);
        const Core::ResourceCommitParams commit{ .m_bindFlags = Core::BarrierAccessFlags::kShaderRead,
                                                 .m_memory = Core::ResourceMemory::kDeviceLocal };
        renderer.GetResourcePool()->CommitBufferMemory(first.Get(), commit);
        renderer.GetResourcePool()->CommitBufferMemory(second.Get(), commit);
        const BufferSRVDescriptor descriptor = descriptors->CreateSRV(first.Get());

        for (uint32_t frame = 0; frame < 8; ++frame)
        {
            View* view = scene->CreateView();
            view->SetEnabled(true);
            renderer.RenderAsync(RenderRequest::CreateForScene(scene, viewport.Get()));
            viewport->m_acquiring->Wait();
            renderer.WaitForScene();
            EXPECT_FALSE(viewport->m_acquiredOnMainThread);
            EXPECT_NE(viewport->m_acquireWorker, 0u);
            EXPECT_EQ(viewport->m_acquireWorker & ~festd::to_underlying(Jobs::FiberAffinityMask::kForegroundWorkers), 0u);

            // Mutate scene membership and publish a descriptor while acquisition is still blocked.
            scene->DestroyView(view);
            const auto* expected = frame == 0 ? first.Get() : second.Get();
            EXPECT_EQ(descriptors->GetPersistentResourceInfo(descriptor.m_value).m_resource.Get(), expected);
            descriptors->Update(descriptor, second.Get());
            EXPECT_EQ(descriptors->GetPersistentResourceInfo(descriptor.m_value).m_resource.Get(), expected);
            // The third scene handoff must also precede reuse waits for the first two GPU frames.
            if (frame == 2)
            {
                EXPECT_FALSE(viewport->m_submissionWait.IsReady());
                gpuHold->Signal(1);
            }

            viewport->m_resume->Signal();
            renderer.WaitForRender();

            viewport->m_acquiring = WaitGroup::Create();
            viewport->m_resume = WaitGroup::Create();
        }

        descriptors->FreePersistentDescriptor(descriptor.m_value);
        renderer.DestroyScene(scene);
        first.Reset();
        second.Reset();
        Renderer::Shutdown();
        device->WaitIdle();
    }
}


#if FE_PLATFORM_WINDOWS
namespace
{
    void PumpWindowEvents()
    {
        MSG message;
        while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }


    void CheckWorkerSurfaceExtent(Core::Device* device)
    {
        const DPI_AWARENESS_CONTEXT previousContext = SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        const HWND window = CreateWindowExW(0,
                                            L"STATIC",
                                            L"DPI extent probe",
                                            WS_OVERLAPPEDWINDOW,
                                            CW_USEDEFAULT,
                                            CW_USEDEFAULT,
                                            640,
                                            480,
                                            nullptr,
                                            nullptr,
                                            GetModuleHandleW(nullptr),
                                            nullptr);
        RECT clientRect{};
        GetClientRect(window, &clientRect);
        SetThreadDpiAwarenessContext(previousContext);
        ASSERT_NE(window, nullptr);
        const auto destroyWindow = festd::defer([&] {
            DestroyWindow(window);
        });

        const VkInstance instance = Vulkan::NativeCast(&Core::DeviceFactory::Get());
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        ASSERT_EQ(Vulkan::CreateSurface(instance, reinterpret_cast<uintptr_t>(window), &surface), VK_SUCCESS);
        const auto destroySurface = festd::defer([&] {
            vkDestroySurfaceKHR(instance, surface, nullptr);
        });
        const Rc<WaitGroup> done = WaitGroup::Create();
        VkSurfaceCapabilitiesKHR capabilities{};
        Jobs::Dispatch(
            Jobs::FiberAffinityMask::kForegroundWorkers,
            festd::span<WaitGroup* const>{},
            Jobs::Priority::kNormal,
            [&] {
                const DPI_AWARENESS_CONTEXT previousWorkerContext = SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_UNAWARE);
                EXPECT_EQ(Vulkan::QuerySurfaceCapabilities(Vulkan::ImplCast(device)->GetNativeAdapter(),
                                                           surface,
                                                           reinterpret_cast<uintptr_t>(window),
                                                           &capabilities),
                          VK_SUCCESS);
                EXPECT_TRUE(AreDpiAwarenessContextsEqual(GetThreadDpiAwarenessContext(), DPI_AWARENESS_CONTEXT_UNAWARE));
                SetThreadDpiAwarenessContext(previousWorkerContext);
            },
            done.Get());
        done->Wait();
        EXPECT_EQ(capabilities.currentExtent.width, static_cast<uint32_t>(clientRect.right));
        EXPECT_EQ(capabilities.currentExtent.height, static_cast<uint32_t>(clientRect.bottom));
    }


    void CheckViewportSuspension(Core::Device* device, Core::ResourcePool* pool, Core::GraphicsQueue* queue,
                                 Core::DescriptorManager* descriptors)
    {
        const HINSTANCE instance = GetModuleHandleW(nullptr);
        WNDCLASSW windowClass{};
        windowClass.lpfnWndProc = DefWindowProcW;
        windowClass.hInstance = instance;
        windowClass.lpszClassName = L"FerrumViewportProbe";
        ASSERT_NE(RegisterClassW(&windowClass), 0);
        const auto unregisterClass = festd::defer([&] {
            UnregisterClassW(windowClass.lpszClassName, instance);
        });
        const HWND window = CreateWindowExW(0,
                                            windowClass.lpszClassName,
                                            L"Viewport probe",
                                            WS_OVERLAPPEDWINDOW,
                                            CW_USEDEFAULT,
                                            CW_USEDEFAULT,
                                            320,
                                            200,
                                            nullptr,
                                            nullptr,
                                            instance,
                                            nullptr);
        ASSERT_NE(window, nullptr);
        const auto destroyWindow = festd::defer([&] {
            DestroyWindow(window);
        });
        ShowWindow(window, SW_SHOWNOACTIVATE);
        PumpWindowEvents();
        Rc<Core::Viewport> viewport = device->CreateViewport(pool, queue);
        viewport->Init({ 320, 200, reinterpret_cast<uintptr_t>(window) });
        Rc<Core::FrameGraph> graph = device->CreateFrameGraph(descriptors, pool, queue);
        const auto renderFrame = [&]() -> bool {
            if (!viewport->AcquireNextImage())
                return false;

            queue->BeginFrame();
            graph->BeginFrame();
            const Vector2UInt resolution(viewport->GetDesc().m_width, viewport->GetDesc().m_height);
            const Rc depth = Core::Texture::Create(device, "ViewportProbeDepth", Core::Format::kD32_SFLOAT, resolution);
            auto* clear = graph->AllocatePassData<Visibility::CoveragePassDesc>();
            clear->m_visibility = Core::TextureView::Create(viewport->GetCurrentColorTarget());
            clear->m_depth = Core::TextureView::Create(depth.Get());
            clear->m_viewport = viewport->GetDesc().GetRect();
            graph->AddPass("ClearViewportProbe", clear, [](Core::FrameGraphContext& context) {
                context.ClearColorTarget(0, Core::ColorClearValue::Float(Color4F(0, 0, 0, 1)));
                context.ClearDepthStencilTarget(0, 0);
                context.ClearRenderTargets();
            });
            viewport->PrepareBlit();
            graph->CompileAndExecute();
            Core::Viewport* presentedViewport = viewport.Get();
            queue->SubmitFrame({ &presentedViewport, 1 });
            pool->EndFrame();
            device->EndFrame();
            return true;
        };

        ASSERT_TRUE(renderFrame());
        ShowWindow(window, SW_MINIMIZE);
        PumpWindowEvents();
        bool suspended = false;
        for (uint32_t attempt = 0; attempt < 8 && !suspended; ++attempt)
        {
            PumpWindowEvents();
            suspended = !renderFrame();
        }

        ASSERT_TRUE(suspended);
        const uint64_t suspendedFrame = Vulkan::ImplCast(queue)->GetFrameIndex();
        EXPECT_FALSE(renderFrame());
        EXPECT_FALSE(renderFrame());
        EXPECT_EQ(Vulkan::ImplCast(queue)->GetFrameIndex(), suspendedFrame);

        ShowWindow(window, SW_RESTORE);
        bool restored = false;
        for (uint32_t attempt = 0; attempt < 8 && !restored; ++attempt)
        {
            PumpWindowEvents();
            restored = renderFrame();
        }

        EXPECT_TRUE(restored);
        device->WaitIdle();
    }
} // namespace
#endif


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
        CheckSplitAsyncUpload(device.Get(), pool.Get(), copyQueue.Get());
        {
            Rc<Core::DescriptorManager> descriptors = device->CreateDescriptorManager();
            Rc<Core::FrameGraph> graph = device->CreateFrameGraph(descriptors.Get(), pool.Get(), queue.Get());
            Rc<Core::PipelineFactory> pipelines = device->CreatePipelineFactory(descriptors.Get());

            Core::ComputePipelineRequest request;
            request.m_desc.SetComputeShader(Env::Name("Shaders/Tests/GpuProbe.cs.hlsl"));
            const auto compute = device->GetComputeCapabilities();
            if (compute.m_waveLaneCountControl && compute.m_minWaveLaneCount <= 32 && compute.m_maxWaveLaneCount >= 32)
                request.m_desc.m_waveLaneCount = 32;

            Core::ComputePipeline* pipeline = pipelines->CreateComputePipeline(request);
            pipeline->GetCompletionWaitGroup()->Wait();
            ASSERT_TRUE(pipeline->IsReady());
            CheckDescriptorSnapshots(device.Get(), pool.Get(), queue.Get(), descriptors.Get(), graph.Get(), pipeline);

            ComputePassRegistry evaluationPipelines;
            ComputePassRegistry shadingPipelines;
            EXPECT_EQ(evaluationPipelines.Register(nullptr), kInvalidIndex);
            EXPECT_EQ(evaluationPipelines.Register(pipeline), 0u);
            const uint32_t registrationRevision = evaluationPipelines.GetRevision();
            EXPECT_EQ(evaluationPipelines.Register(pipeline), 0u);
            EXPECT_EQ(evaluationPipelines.GetRevision(), registrationRevision);

            Core::ComputePipelineRequest shadingRequest = request;
            shadingRequest.m_defines = Env::Name("SHADE_PASS=1");
            Core::ComputePipeline* shadingPipeline = pipelines->CreateComputePipeline(shadingRequest);
            shadingPipeline->GetCompletionWaitGroup()->Wait();
            ASSERT_TRUE(shadingPipeline->IsReady());
            EXPECT_NE(shadingPipeline, pipeline);
            EXPECT_EQ(shadingPipelines.Register(shadingPipeline), 0u);
            EXPECT_EQ(shadingPipelines.Register(shadingPipeline), 0u);
            EXPECT_EQ(shadingPipelines.Register(pipeline), 1u);
            EXPECT_EQ(evaluationPipelines.GetSlots().size(), 1u);
            EXPECT_EQ(shadingPipelines.GetSlots().size(), 2u);

            evaluationPipelines.Release(0);
            EXPECT_EQ(evaluationPipelines.GetRevision(), registrationRevision);
            EXPECT_EQ(evaluationPipelines.GetSlots().size(), 1u);
            evaluationPipelines.Release(0);
            EXPECT_EQ(evaluationPipelines.GetRevision(), registrationRevision + 1);
            EXPECT_TRUE(evaluationPipelines.GetSlots().empty());
            EXPECT_EQ(evaluationPipelines.Register(shadingPipeline), 0u);
            EXPECT_EQ(evaluationPipelines.Register(pipeline), 1u);
            evaluationPipelines.Release(0);
            EXPECT_EQ(evaluationPipelines.GetSlots()[1].m_pipeline.Get(), pipeline);
            EXPECT_EQ(evaluationPipelines.Register(shadingPipeline), 0u);

            if (compute.m_waveLaneCountControl)
            {
                Core::ComputePipelineRequest waveRequest = request;
                waveRequest.m_desc.m_waveLaneCount = request.m_desc.m_waveLaneCount != 0 ? 0 : compute.m_waveLaneCount;
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
            // Reserved views without shader consumers must not bind transfer-only storage to a shader descriptor.
            (void)graph->GetSRV(readback.Get());
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
            Core::GraphicsPipelineRequest coverageRequest;
            coverageRequest.m_desc.SetAmplificationShader(Env::Name("Shaders/Passes/MeshPass/MeshCull.as.hlsl"))
                .SetMeshShader(Env::Name("Shaders/Passes/Visibility/DepthVisibility.ms.hlsl"))
                .SetPixelShader(Env::Name("Shaders/Passes/Visibility/DepthVisibility.ps.hlsl"))
                .SetRTVFormat(Core::Format::kR32G32_UINT)
                .SetDSVFormat(Core::Format::kD32_SFLOAT);
            Core::GraphicsPipeline* coveragePipeline = pipelines->CreateGraphicsPipeline(coverageRequest);
            coveragePipeline->GetCompletionWaitGroup()->Wait();
            EXPECT_TRUE(coveragePipeline->IsReady());

            Core::ComputePipelineRequest reconstructionRequest;
            reconstructionRequest.m_desc.SetComputeShader(Env::Name("Shaders/Tests/ReconstructionProbe.cs.hlsl"));
            Core::ComputePipeline* reconstructionPipeline = pipelines->CreateComputePipeline(reconstructionRequest);
            reconstructionPipeline->GetCompletionWaitGroup()->Wait();
            ASSERT_TRUE(reconstructionPipeline->IsReady());
            Rc<Core::Buffer> reconstructed = Core::Buffer::CreateByteAddress(device.Get(), "ReconstructionProbe", 192);
            Rc<Core::Buffer> reconstructionReadback =
                Core::Buffer::CreateByteAddress(device.Get(), "ReconstructionReadback", 192);
            pool->CommitBufferMemory(reconstructionReadback.Get(), commit);
            queue->BeginFrame();
            graph->BeginFrame();
            auto* reconstruction = graph->AllocatePassData<Stage1Probe::PassDesc>();
            reconstruction->m_pipeline = reconstructionPipeline;
            reconstruction->m_constants.m_words = graph->GetUAV(reconstructed.Get());
            graph->AddDispatchPass("CheckPerspectiveReconstruction", reconstruction, Core::ComputeWorkGroupCount(4));
            graph->AddCopyPass(Core::BufferView(reconstructionReadback.Get()), Core::BufferView(reconstructed.Get()));
            graph->CompileAndExecute();
            Vulkan::CommandBuffer* reconstructionCommands = Vulkan::ImplCast(queue.Get())->GetCurrentCommandBuffer();
            const Core::FenceSyncPoint reconstructionFence = queue->CloseFrame();
            reconstructionCommands->EnqueueFenceToSignal(reconstructionFence);
            reconstructionCommands->Submit();
            reconstructionFence.Wait();
            device->WaitIdle();
            const uint32_t* results = static_cast<const uint32_t*>(reconstructionReadback->Map());
            reconstructionReadback->InvalidateMappedRange(0, 192);
            for (uint32_t triangle = 0; triangle < 3; ++triangle)
            {
                const uint32_t* record = results + triangle * 12;
                EXPECT_EQ(record[0], 1u);
                const float w0 = std::bit_cast<float>(record[1]);
                const float w1 = std::bit_cast<float>(record[2]);
                const float w2 = std::bit_cast<float>(record[3]);
                EXPECT_NEAR(w0 + w1 + w2, 1.0f, 1e-6f);
                EXPECT_LT(std::bit_cast<float>(record[10]), 5e-6f);
                EXPECT_LT(std::bit_cast<float>(record[11]), 5e-6f);
            }

            EXPECT_EQ(results[36], 0u);
            EXPECT_NEAR(std::bit_cast<float>(results[1]), 0.25f / 0.525f, 1e-6f);
            EXPECT_NEAR(std::bit_cast<float>(results[2]), 0.175f / 0.525f, 1e-6f);
            EXPECT_NEAR(std::bit_cast<float>(results[3]), 0.1f / 0.525f, 1e-6f);
            reconstructionReadback->Unmap();
            RasterProbe::Check(device.Get(), pool.Get(), queue.Get(), descriptors.Get(), pipelines.Get());
            RasterProbe::Check(device.Get(), pool.Get(), queue.Get(), descriptors.Get(), pipelines.Get(), 64);
            SurfaceProbe::Check(device.Get(), pool.Get(), queue.Get(), descriptors.Get(), pipelines.Get());
            SurfaceProbe::Check(device.Get(), pool.Get(), queue.Get(), descriptors.Get(), pipelines.Get(), true);
            DeferredProbe::Check(device.Get(), pool.Get(), queue.Get(), descriptors.Get(), pipelines.Get());
#if FE_PLATFORM_WINDOWS
            CheckWorkerSurfaceExtent(device.Get());
            CheckViewportSuspension(device.Get(), pool.Get(), queue.Get(), descriptors.Get());
#endif
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
