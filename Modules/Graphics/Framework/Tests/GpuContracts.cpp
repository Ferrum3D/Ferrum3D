#include "ConstantsProbe.h"
#include "DeferredProbe.h"
#include "DepthPyramidProbe.h"
#include "GpuProbe.h"
#include "RasterProbe.h"
#include "SunShadowProbe.h"
#include "SurfaceProbe.h"
#include <Core/Base/PlatformInclude.h>
#include <Core/Jobs/Jobs.h>
#include <Core/Math/Packing.h>
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
#include <Graphics/Features/Lighting/LocalLightingSceneModule.h>
#include <Graphics/Features/Shadows/LocalShadows.h>
#include <Graphics/Materials/MaterialInstance.h>
#include <Graphics/Passes/VisibilityPass.h>
#include <Graphics/Renderer.h>
#include <Graphics/Scene/ComputePassRegistry.h>
#include <Graphics/Scene/Scene.h>
#include <Graphics/Scene/View.h>
#include <Graphics/Tables/MeshGroupTable.h>
#include <Graphics/Tables/MeshInstanceTable.h>
#include <Shaders/Lighting/LocalLighting.h>
#include <Shaders/Scene/ViewData.h>
#include <cmath>
#include <gtest/gtest.h>

using namespace FE;
using namespace FE::Graphics;

namespace
{
    // Shader data names buffers, constants and tables by type, so these mistakes do not compile.
    static_assert(!std::is_assignable_v<StructuredBufferDescriptor<uint32_t>&, StructuredBufferDescriptor<float>>);
    static_assert(!std::is_assignable_v<StructuredBufferDescriptor<uint32_t>&, RWStructuredBufferDescriptor<uint32_t>>);
    static_assert(!std::is_assignable_v<StructuredBufferDescriptor<uint32_t>&, BufferSRVDescriptor>);
    static_assert(!std::is_assignable_v<ConstantsPtr<ViewData>&, ConstantsPtr<GlobalLightingData>>);
    static_assert(!std::is_assignable_v<DB::TableAddress<MeshInstanceTable>&, DB::TableAddress<MeshGroupTable>>);
    static_assert(!std::is_convertible_v<Core::StructuredBufferPtr<uint32_t>, Core::StructuredBufferPtr<float>>);


    //! Nested and type-erased constants declare the accesses of their descriptors, across the chunks of several frames.
    void CheckConstants(Core::Device* device, Core::ResourcePool* pool, Core::GraphicsQueue* queue,
                        Core::DescriptorManager* descriptors, Core::PipelineFactory* pipelines)
    {
        Rc<Core::FrameGraph> graph = device->CreateFrameGraph(descriptors, pool, queue);

        Core::ComputePipelineRequest request;
        request.m_desc.SetComputeShader(Env::Name("Shaders/Tests/ConstantsProbe.cs.hlsl"));
        Core::ComputePipeline* pipeline = pipelines->CreateComputePipeline(request);
        pipeline->GetCompletionWaitGroup()->Wait();
        ASSERT_TRUE(pipeline->IsReady());

        constexpr uint32_t kByteSize = 2 * sizeof(uint32_t);
        Rc<Core::Buffer> readback = Core::Buffer::CreateByteAddress(device, "ConstantsProbeReadback", kByteSize);
        Core::ResourceCommitParams commit;
        commit.m_memory = Core::ResourceMemory::kHostRandomAccess;
        commit.m_bindFlags = Core::BarrierAccessFlags::kCopyDest;
        pool->CommitBufferMemory(readback.Get(), commit);

        for (uint32_t frame = 0; frame < 4; ++frame)
        {
            SCOPED_TRACE(frame);

            queue->BeginFrame();
            graph->BeginFrame();

            auto output = Core::Buffer::CreateStructured<uint32_t>(device, "ConstantsProbeOutput", 2);
            ConstantsProbe::Inner inner{};
            inner.m_output = graph->GetUAV(output.Get());
            inner.m_value = frame + 2;

            // Enough to fill the first chunk, so that the constants of the pass span several of them.
            constexpr uint32_t kFillerCount = 8192;
            const ViewData filler{};
            for (uint32_t fillerIndex = 0; fillerIndex < kFillerCount; ++fillerIndex)
                (void)graph->CreateConstants(filler);

            ConstantsProbe::Outer outer{};
            outer.m_inner = graph->CreateConstants(inner);
            outer.m_multiplier = 3;

            ConstantsProbe::Inner erased = inner;
            erased.m_value = frame + 7;

            ConstantsProbe::Constants constants{};
            constants.m_outer = graph->CreateConstants(outer);
            constants.m_erased = graph->CreateConstants(erased);
            graph->AddDispatch("ConstantsProbe", pipeline, constants, Core::ComputeWorkGroupCount(1));
            graph->AddCopyPass(Core::BufferView(readback.Get()), Core::BufferView(output.Get()));
            graph->CompileAndExecute();

            const Core::FenceSyncPoint completion = queue->GetCurrentFence();
            queue->SubmitFrame();
            completion.Wait();

            const auto* words = static_cast<const uint32_t*>(readback->Map());
            readback->InvalidateMappedRange(0, kByteSize);
            EXPECT_EQ(words[0], (frame + 2) * 3);
            EXPECT_EQ(words[1], frame + 7);
            readback->Unmap();
        }

        device->WaitIdle();
    }


    void CheckLightClusterMasks(Core::Device* device, Core::ResourcePool* pool, Core::GraphicsQueue* queue,
                                Core::DescriptorManager* descriptors, Core::PipelineFactory* pipelines)
    {
        DB::Database database(device, pool);
        LocalLightingSceneModule lightsModule(nullptr, &database);
        LocalLightData lights[34]{};
        LightHandle handles[34];
        for (uint32_t index = 0; index < 34; ++index)
        {
            lights[index] = PackLocalLightData(Vector3::kZero, 100000, Vector3(0.2f, 0.6f, 4), 1e12f);
        }

        lights[33] = PackLocalLightData(Vector3(1000000, 0, 0),
                                        1,
                                        Vector3(0.2f, 0.6f, 4),
                                        1e12f,
                                        Math::Normalize(Vector3(1, -2, -3)),
                                        0.005f,
                                        0.01f);
        for (uint32_t index = 0; index < 34; ++index)
            handles[index] = lightsModule.CreateLight(lights[index]);

        LightClusterData clusters{};
        clusters.m_view = Matrix4x4::kIdentity;
        clusters.m_projection = Matrix4x4::Projection(1, 1, 0.1f, 100);
        ViewData clusterView{};
        clusterView.m_inverseViewProjection = Math::Invert(clusters.m_projection);
        clusterView.m_viewport = { 0, 0, 17, 19 };
        clusters.m_depth = { 0.1f, 100, kLightClusterSlices / std::log2(1000.0f), 0 };
        clusters.m_tiles = Vector2UInt(1, 1);
        clusters.m_lightCount = 34;
        clusters.m_wordCount = 2;
        const uint32_t bytes = kLightClusterSlices * 2 * sizeof(uint32_t);
        auto masks = Core::Buffer::CreateStructured<uint32_t>(device, "ProbeMasks", bytes / sizeof(uint32_t));
        const auto readback = Core::Buffer::CreateByteAddress(device, "ProbeMasksReadback", bytes);
        Core::ResourceCommitParams commit;
        commit.m_memory = Core::ResourceMemory::kHostRandomAccess;
        commit.m_bindFlags = Core::BarrierAccessFlags::kCopyDest;
        pool->CommitBufferMemory(readback.Get(), commit);
        const uint32_t decodedBytes = 34 * 16 * sizeof(float);
        auto decoded = Core::Buffer::CreateStructured<uint32_t>(device, "DecodedLights", decodedBytes / sizeof(uint32_t));
        const auto decodedReadback = Core::Buffer::CreateByteAddress(device, "DecodedLightsReadback", decodedBytes);
        pool->CommitBufferMemory(decodedReadback.Get(), commit);
        Core::ComputePipelineRequest decodeRequest;
        decodeRequest.m_desc.SetComputeShader("Shaders/Tests/GpuProbe.cs.hlsl");
        auto* decodePipeline = pipelines->CreateComputePipeline(decodeRequest);
        decodePipeline->GetCompletionWaitGroup()->Wait();
        ASSERT_TRUE(decodePipeline->IsReady());
        Core::ComputePipelineRequest request;
        request.m_desc.SetComputeShader("Shaders/Lighting/CullLocalLights.cs.hlsl");
        auto* pipeline = pipelines->CreateComputePipeline(request);
        pipeline->GetCompletionWaitGroup()->Wait();
        ASSERT_TRUE(pipeline->IsReady());
        auto graph = device->CreateFrameGraph(descriptors, pool, queue);
        Core::RingUploader uploader;
        uploader.Setup("LightProbeUploader", pool, 64 * 1024);
        const LightHandle stale0 = handles[0];
        const LightHandle stale32 = handles[32];
        for (uint32_t frame = 0; frame < 4; ++frame)
        {
            SCOPED_TRACE(frame);
            if (frame == 2)
            {
                lightsModule.DestroyLight(handles[0]);
                lightsModule.DestroyLight(handles[32]);
                EXPECT_EQ(lightsModule.FindLight(stale0), nullptr);
                EXPECT_EQ(lightsModule.FindLight(stale32), nullptr);
            }
            else if (frame == 3)
            {
                handles[0] = lightsModule.CreateLight(lights[33]);
                handles[32] = lightsModule.CreateLight(lights[32]);
                EXPECT_EQ(handles[0].m_value, stale0.m_value);
                EXPECT_NE(handles[0].m_generation, stale0.m_generation);
                EXPECT_EQ(handles[32].m_value, stale32.m_value);
                EXPECT_NE(handles[32].m_generation, stale32.m_generation);
                lightsModule.UpdateLight(stale0, lights[0]);
                lightsModule.DestroyLight(stale32);
                lightsModule.UpdateLight(handles[1], lights[33]);
            }

            queue->BeginFrame();
            graph->BeginFrame();
            database.Update(*graph, queue->GetCurrentFence());
            CullLocalLightsConstants pass{};
            pass.m_lighting.m_lights = lightsModule.GetLightTable()->GetInstance();
            clusters.m_viewData = graph->CreateConstants(clusterView);
            pass.m_lighting.m_clusters = graph->CreateConstants(clusters);
            pass.m_masks = graph->GetUAV(masks.Get());
            graph->AddDispatch("ProbeClusterMasks", pipeline, pass, 1);
            graph->AddCopyPass(Core::BufferView(readback.Get()), Core::BufferView(masks.Get()));
            if (frame == 0)
            {
                Stage1Probe::Constants decode{};
                decode.m_material = lightsModule.GetLightTable()->GetInstance().m_pageTableAddress;
                decode.m_writeArguments = 3;
                decode.m_words = graph->GetUAV(decoded.Get());
                graph->AddDispatch("DecodePackedLights", decodePipeline, decode, 34);
                graph->AddCopyPass(Core::BufferView(decodedReadback.Get()), Core::BufferView(decoded.Get()));
            }

            graph->CompileAndExecute();
            const auto completion = queue->GetCurrentFence();
            uploader.CloseFrame(completion);
            queue->SubmitFrame();
            completion.Wait();
            const auto* words = static_cast<const uint32_t*>(readback->Map());
            readback->InvalidateMappedRange(0, bytes);
            for (uint32_t slice = 0; slice < kLightClusterSlices; ++slice)
            {
                const uint32_t firstWord = frame < 2 ? 0xffffffffu : frame == 2 ? 0xfffffffeu : 0xfffffffcu;
                EXPECT_EQ(words[slice * 2], firstWord);
                EXPECT_EQ(words[slice * 2 + 1], frame == 2 ? 0u : 1u);
            }

            readback->Unmap();
            if (frame == 0)
            {
                const auto* values = static_cast<const float*>(decodedReadback->Map());
                decodedReadback->InvalidateMappedRange(0, decodedBytes);
                for (uint32_t index = 0; index < 34; ++index)
                {
                    const float* record = values + index * 16;
                    EXPECT_EQ(record[0], lights[index].m_positionRange.x);
                    EXPECT_EQ(record[3], lights[index].m_positionRange.w);
                    const Vector3 color = Math::Pack::R11G11B10FloatToRGB32Float(lights[index].m_color);
                    for (uint32_t channel = 0; channel < 3; ++channel)
                        EXPECT_EQ(record[4 + channel], color.m_values[channel]);

                    EXPECT_EQ(record[7], lights[index].m_colorMultiplier);
                    const Vector3 direction = Math::Pack::Octahedral16SnormToNormal(lights[index].m_direction);
                    for (uint32_t channel = 0; channel < 3; ++channel)
                        EXPECT_NEAR(record[8 + channel], direction.m_values[channel], 0.000001f);

                    if (index < 33)
                    {
                        EXPECT_EQ(record[11], -1);
                    }
                    else
                    {
                        EXPECT_NEAR(record[11], std::cos(0.01f), 0.000001f);
                        EXPECT_NEAR(record[12], std::cos(0.005f), 0.000001f);
                        EXPECT_LT(record[11], record[12]);
                    }
                }

                decodedReadback->Unmap();
            }
        }

        for (const LightHandle handle : handles)
            lightsModule.DestroyLight(handle);

        EXPECT_EQ(lightsModule.GetLightSlotCount(), 0u);
        graph.Reset();
        uploader.Shutdown();
    }


    uint32_t CountMapsOf(const LocalShadows::MapAllocation& allocation, const LightHandle light)
    {
        uint32_t count = 0;
        for (const LocalShadows::MapSetup& map : allocation.m_maps)
            count += map.m_isUsed && map.m_light == light ? 1 : 0;

        return count;
    }


    void CheckLocalShadowSelection(Core::Device* device, Core::ResourcePool* pool)
    {
        DB::Database database(device, pool);
        LocalLightingSceneModule lights(nullptr, &database);

        // The camera looks along +Z from the origin.
        LocalShadows::CameraDesc camera;
        camera.m_frustum = Frustum(Matrix4x4::Projection(1.5f, 1.0f, 0.1f, 100.0f));
        camera.m_position = Vector3::kZero;

        const Vector3 down(0, -1, 0);
        const LightHandle nearPoint = lights.CreateLight(PackLocalLightData(Vector3(0, 0, 5), 2, Vector3(1), 100));
        const LightHandle farSpot =
            lights.CreateLight(PackLocalLightData(Vector3(0, 0, 40), 4, Vector3(1), 100, down, 0.3f, 0.5f));
        const LightHandle hiddenPoint = lights.CreateLight(PackLocalLightData(Vector3(1000, 0, 0), 2, Vector3(1), 100));
        const LightHandle unshadowedSpot =
            lights.CreateLight(PackLocalLightData(Vector3(0, 0, 10), 4, Vector3(1), 100, down, 0.3f, 0.5f));
        for (const LightHandle light : { nearPoint, farSpot, hiddenPoint })
            lights.SetLightCastsShadows(light, true);

        // The point light covers more of the screen, so it takes the first six maps; lights out of view or without shadows
        // take none, and every new map discards its pages.
        LocalShadows::MapAllocation allocation;
        EXPECT_EQ(LocalShadows::SelectMaps(lights, camera, allocation), 0x7Fu);
        EXPECT_EQ(CountMapsOf(allocation, nearPoint), kLocalShadowPointFaceCount);
        EXPECT_EQ(allocation.m_maps[6].m_light, farSpot);
        EXPECT_EQ(CountMapsOf(allocation, hiddenPoint), 0u);
        EXPECT_EQ(CountMapsOf(allocation, unshadowedSpot), 0u);
        for (uint32_t face = 0; face < kLocalShadowPointFaceCount; ++face)
            EXPECT_EQ(allocation.m_maps[face].m_face, face);

        // Unchanged lights keep their maps and pages; a moved light discards only the pages of its map.
        EXPECT_EQ(LocalShadows::SelectMaps(lights, camera, allocation), 0u);
        lights.UpdateLight(farSpot, PackLocalLightData(Vector3(0, 1, 40), 4, Vector3(1), 100, down, 0.3f, 0.5f));
        EXPECT_EQ(LocalShadows::SelectMaps(lights, camera, allocation), 1u << 6);
        EXPECT_EQ(allocation.m_maps[6].m_light, farSpot);

        // A light that stops casting shadows releases its maps; the others keep theirs.
        lights.SetLightCastsShadows(nearPoint, false);
        EXPECT_EQ(LocalShadows::SelectMaps(lights, camera, allocation), 0x3Fu);
        EXPECT_EQ(CountMapsOf(allocation, nearPoint), 0u);
        EXPECT_EQ(allocation.m_maps[6].m_light, farSpot);

        // Three point lights exceed the budget together with the spot light; the one covering the least is left out.
        festd::fixed_vector<LightHandle, 3> points;
        for (uint32_t index = 0; index < 3; ++index)
        {
            const float distance = 6.0f + 4.0f * index;
            points.push_back(lights.CreateLight(PackLocalLightData(Vector3(0, 0, distance), 2, Vector3(1), 100)));
            lights.SetLightCastsShadows(points.back(), true);
        }

        LocalShadows::SelectMaps(lights, camera, allocation);
        EXPECT_EQ(CountMapsOf(allocation, points[0]), kLocalShadowPointFaceCount);
        EXPECT_EQ(CountMapsOf(allocation, points[1]), kLocalShadowPointFaceCount);
        EXPECT_EQ(CountMapsOf(allocation, points[2]), 0u);
        EXPECT_EQ(CountMapsOf(allocation, farSpot), 1u);

        for (const LightHandle light : { nearPoint, farSpot, hiddenPoint, unshadowedSpot })
            lights.DestroyLight(light);
        for (const LightHandle light : points)
            lights.DestroyLight(light);
    }


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
            Stage1Probe::Constants pass{};
            pass.m_words = RWStructuredBufferDescriptor<uint32_t>(descriptor);
            pass.m_writeArguments = 0;
            graph->AddDispatch("FrozenDescriptorProbe", pipeline, pass, Core::ComputeWorkGroupCount(1));
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
        void DoRelease() override
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
            auto output = Core::Buffer::CreateStructured<uint32_t>(device.Get(), "IndirectProbeOutput", 3);
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
            Stage1Probe::Constants write{};
            write.m_words =
                RWStructuredBufferDescriptor<uint32_t>(graph->GetUAV(Core::BufferView(arguments.Get()).Slice(16, 12)));
            write.m_writeArguments = 1;
            graph->AddDispatch("WriteIndirectArguments", pipeline, write, Core::ComputeWorkGroupCount(1));

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
            coverageRequest.m_desc.SetMeshShader(Env::Name("Shaders/Passes/Visibility/DepthVisibility.ms.hlsl"))
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
            auto reconstructed = Core::Buffer::CreateStructured<uint32_t>(device.Get(), "ReconstructionProbe", 48);
            Rc<Core::Buffer> reconstructionReadback =
                Core::Buffer::CreateByteAddress(device.Get(), "ReconstructionReadback", 192);
            pool->CommitBufferMemory(reconstructionReadback.Get(), commit);
            queue->BeginFrame();
            graph->BeginFrame();
            Stage1Probe::Constants reconstruction{};
            reconstruction.m_words = graph->GetUAV(reconstructed.Get());
            graph->AddDispatch("CheckPerspectiveReconstruction",
                               reconstructionPipeline,
                               reconstruction,
                               Core::ComputeWorkGroupCount(4));
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
            CheckConstants(device.Get(), pool.Get(), queue.Get(), descriptors.Get(), pipelines.Get());
            CheckLightClusterMasks(device.Get(), pool.Get(), queue.Get(), descriptors.Get(), pipelines.Get());
            CheckLocalShadowSelection(device.Get(), pool.Get());
            RasterProbe::Check(device.Get(), pool.Get(), queue.Get(), descriptors.Get(), pipelines.Get());
            RasterProbe::Check(device.Get(), pool.Get(), queue.Get(), descriptors.Get(), pipelines.Get(), 64);
            SurfaceProbe::Check(device.Get(), pool.Get(), queue.Get(), descriptors.Get(), pipelines.Get());
            SurfaceProbe::Check(device.Get(), pool.Get(), queue.Get(), descriptors.Get(), pipelines.Get(), true);
            DeferredProbe::Check(device.Get(), pool.Get(), queue.Get(), descriptors.Get(), pipelines.Get());
            DepthPyramidProbe::Check(device.Get(), pool.Get(), queue.Get(), descriptors.Get(), pipelines.Get());
            SunShadowProbe::Check(device.Get(), pool.Get(), queue.Get(), descriptors.Get(), pipelines.Get());
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
