#include "ScreenTraceProbe.h"
#include <Graphics/Core/DescriptorManager.h>
#include <Graphics/Core/GraphicsQueue.h>
#include <Graphics/Core/PipelineFactory.h>
#include <Graphics/Core/PipelineVariantSet.h>
#include <Graphics/Core/ResourcePool.h>
#include <Graphics/Core/Vulkan/GraphicsQueue.h>
#include <gtest/gtest.h>

namespace FE::Graphics::ScreenTraceProbe
{
    namespace
    {
        const Vector2UInt kDepthSize(320, 200);


        //! A perspective camera away from the origin that looks along none of the axes.
        ViewData CreateView()
        {
            const Matrix4x4 viewMatrix =
                Matrix4x4::LookAt(Vector3(3.0f, 2.0f, -5.0f), Vector3(0.4f, -0.2f, 1.0f), Vector3::AxisY());
            const float aspectRatio = float(kDepthSize.x) / float(kDepthSize.y);
            const Matrix4x4 projection = Matrix4x4::Projection(1.0f, aspectRatio, 0.1f, 1000.0f);

            ViewData view{};
            view.m_viewProjection = viewMatrix * projection;
            view.m_inverseViewProjection = Math::Invert(view.m_viewProjection);
            view.m_unjitteredViewProjection = view.m_viewProjection;
            view.m_previousViewProjection = view.m_viewProjection;

            const Vector4 cameraPosition = Math::Invert(viewMatrix).m_rows[3];
            view.m_cameraPosition = PackedVector4F(cameraPosition.x, cameraPosition.y, cameraPosition.z, 1.0f);
            view.m_viewport = PackedVector4F(0.0f, 0.0f, float(kDepthSize.x), float(kDepthSize.y));
            return view;
        }


        void CheckResults(const uint32_t* results)
        {
            const uint32_t pixelCount = kDepthSize.x * kDepthSize.y;

            uint32_t occludedCount = 0;
            uint32_t mismatchCount = 0;
            for (uint32_t pixelIndex = 0; pixelIndex < pixelCount; ++pixelIndex)
            {
                const bool isOccluded = (results[pixelIndex] & kTraceOccluded) != 0;
                const bool isReferenceOccluded = (results[pixelIndex] & kReferenceOccluded) != 0;
                if (isReferenceOccluded)
                    ++occludedCount;

                if (isOccluded != isReferenceOccluded)
                    ++mismatchCount;
            }

            // Both outcomes are common, so agreement is not trivial.
            EXPECT_GT(occludedCount, pixelCount / 20);
            EXPECT_LT(occludedCount, pixelCount - pixelCount / 20);

            // The implementations round differently, which decides steps that lie at a threshold or at a pixel border.
            EXPECT_LT(mismatchCount, pixelCount / 500);
        }
    } // namespace


    void Check(Core::Device* device, Core::ResourcePool* pool, Core::GraphicsQueue* queue, Core::DescriptorManager* descriptors,
               Core::PipelineFactory* pipelines)
    {
        Core::ComputePipelineRequest request;
        request.m_desc.SetComputeShader("Shaders/Tests/ScreenTraceProbe.cs.hlsl");
        Core::ComputePipeline* pipeline = pipelines->CreateComputePipeline(request);
        pipeline->GetCompletionWaitGroup()->Wait();
        ASSERT_TRUE(pipeline->IsReady());

        Rc<Core::FrameGraph> graph = device->CreateFrameGraph(descriptors, pool, queue);

        queue->BeginFrame();
        graph->BeginFrame();

        Rc<Core::Texture> depth = Core::Texture::Create(device, "ScreenTraceProbeDepth", Core::Format::kR32_SFLOAT, kDepthSize);

        Constants fill{};
        fill.m_target = graph->GetUAV(depth.Get());
        fill.m_mode = kFillMode;
        fill.m_size = kDepthSize;
        graph->AddDispatch("FillScreenTraceProbe", pipeline, fill, Math::CeilDivide(kDepthSize, kThreadGroupSize));

        const uint32_t pixelCount = kDepthSize.x * kDepthSize.y;
        const uint32_t byteSize = pixelCount * sizeof(uint32_t);
        auto output = Core::Buffer::CreateStructured<uint32_t>(device, "ScreenTraceProbeOutput", pixelCount);
        Rc<Core::Buffer> readback = Core::Buffer::CreateByteAddress(device, "ScreenTraceProbeReadback", byteSize);
        Core::ResourceCommitParams readbackCommit;
        readbackCommit.m_memory = Core::ResourceMemory::kHostRandomAccess;
        readbackCommit.m_bindFlags = Core::BarrierAccessFlags::kCopyDest;
        pool->CommitBufferMemory(readback.Get(), readbackCommit);

        Constants trace{};
        trace.m_view = graph->CreateConstants(CreateView());
        trace.m_depth = graph->GetSRV(depth.Get());
        trace.m_output = graph->GetUAV(output.Get());
        trace.m_mode = kTraceMode;
        trace.m_size = kDepthSize;
        graph->AddDispatch("TraceScreenTraceProbe", pipeline, trace, Math::CeilDivide(kDepthSize, kThreadGroupSize));
        graph->AddCopyPass(Core::BufferView(readback.Get()), Core::BufferView(output.Get()));

        graph->CompileAndExecute();
        Vulkan::CommandBuffer* commands = Vulkan::ImplCast(queue)->GetCurrentCommandBuffer();
        const Core::FenceSyncPoint fence = queue->CloseFrame();
        commands->EnqueueFenceToSignal(fence);
        commands->Submit();
        fence.Wait();
        device->WaitIdle();

        const auto* results = static_cast<const uint32_t*>(readback->Map());
        readback->InvalidateMappedRange(0, byteSize);
        CheckResults(results);
        readback->Unmap();

        graph.Reset();
    }
} // namespace FE::Graphics::ScreenTraceProbe
