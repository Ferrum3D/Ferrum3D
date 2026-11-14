#include "ScreenShadowProbe.h"
#include <Graphics/Core/DescriptorManager.h>
#include <Graphics/Core/GraphicsQueue.h>
#include <Graphics/Core/PipelineFactory.h>
#include <Graphics/Core/PipelineVariantSet.h>
#include <Graphics/Core/ResourcePool.h>
#include <Graphics/Core/Vulkan/GraphicsQueue.h>
#include <Graphics/Features/Shadows/ScreenSpaceShadow.h>
#include <gtest/gtest.h>

namespace FE::Graphics::ScreenShadowProbe
{
    namespace
    {
        // Not a multiple of the wave size, so wavefronts cross every border of the depth.
        const Vector2UInt kDepthSize(320, 200);
        const Vector2UInt kSlabMin(150, 90);
        const Vector2UInt kSlabMax(170, 110);


        //! A light in the plane of the floor, so that its rays graze the floor and pass behind the slab.
        struct LightCase final
        {
            const char* m_name = nullptr;
            Vector4 m_clipPosition = Vector4::kZero;
            //! A floor pixel next to the slab on the side away from the light, and one on the side of the light.
            Vector2UInt m_shadowedPixel = Vector2UInt::kZero;
            Vector2UInt m_litPixel = Vector2UInt::kZero;
            //! Every shadowed pixel lies in this rectangle, which extends from the slab away from the light.
            Vector2UInt m_shadowMin = Vector2UInt::kZero;
            Vector2UInt m_shadowMax = Vector2UInt::kZero;
        };


        void CheckVisibility(const LightCase& light, const float* visibility)
        {
            const auto load = [visibility](const Vector2UInt pixel) {
                return visibility[pixel.y * kDepthSize.x + pixel.x];
            };

            EXPECT_LT(load(light.m_shadowedPixel), 0.25f);
            EXPECT_GT(load(light.m_litPixel), 0.75f);

            // The slab does not shadow itself.
            EXPECT_GT(load(Vector2UInt(160, 100)), 0.75f);

            uint32_t shadowedCount = 0;
            uint32_t strayCount = 0;
            for (uint32_t y = 0; y < kDepthSize.y; ++y)
            {
                for (uint32_t x = 0; x < kDepthSize.x; ++x)
                {
                    if (load(Vector2UInt(x, y)) >= 0.5f)
                        continue;

                    const bool isInside = x >= light.m_shadowMin.x && x < light.m_shadowMax.x && y >= light.m_shadowMin.y
                        && y < light.m_shadowMax.y;
                    if (isInside)
                    {
                        ++shadowedCount;
                        continue;
                    }

                    if (strayCount < 8)
                        ADD_FAILURE() << "pixel (" << x << ", " << y << ") is shadowed: " << load(Vector2UInt(x, y));

                    ++strayCount;
                }
            }

            // The shadow is as wide as the slab and most of the sample count long.
            EXPECT_GT(shadowedCount, 600u);
            EXPECT_EQ(strayCount, 0u);
        }


        void CheckLight(Core::Device* device, Core::ResourcePool* pool, Core::GraphicsQueue* queue,
                        Core::DescriptorManager* descriptors, Core::ComputePipeline* pipeline, const LightCase& light)
        {
            SCOPED_TRACE(light.m_name);

            Rc<Core::FrameGraph> graph = device->CreateFrameGraph(descriptors, pool, queue);

            queue->BeginFrame();
            graph->BeginFrame();

            Rc<Core::Texture> depth =
                Core::Texture::Create(device, "ScreenShadowProbeDepth", Core::Format::kR32_SFLOAT, kDepthSize);

            Constants fill{};
            fill.m_target = graph->GetUAV(depth.Get());
            fill.m_mode = kFillMode;
            fill.m_size = kDepthSize;
            fill.m_slabMin = kSlabMin;
            fill.m_slabMax = kSlabMax;
            graph->AddDispatch("FillScreenShadowProbe", pipeline, fill, Math::CeilDivide(kDepthSize, kThreadGroupSize));

            ScreenSpaceShadow::LightDesc lightDesc;
            lightDesc.m_clipPosition = light.m_clipPosition;

            const Rc<Core::Texture> visibility = ScreenSpaceShadow::AddLightPass(*graph, depth.Get(), lightDesc);
            ASSERT_NE(visibility, nullptr);
            ASSERT_EQ(visibility->GetDesc().GetSize2D(), kDepthSize);

            const uint32_t pixelCount = kDepthSize.x * kDepthSize.y;
            const uint32_t byteSize = pixelCount * sizeof(float);
            auto output = Core::Buffer::CreateStructured<float>(device, "ScreenShadowProbeOutput", pixelCount);
            Rc<Core::Buffer> readback = Core::Buffer::CreateByteAddress(device, "ScreenShadowProbeReadback", byteSize);
            Core::ResourceCommitParams readbackCommit;
            readbackCommit.m_memory = Core::ResourceMemory::kHostRandomAccess;
            readbackCommit.m_bindFlags = Core::BarrierAccessFlags::kCopyDest;
            pool->CommitBufferMemory(readback.Get(), readbackCommit);

            Constants read{};
            read.m_source = graph->GetSRV(visibility.Get());
            read.m_output = graph->GetUAV(output.Get());
            read.m_mode = kReadMode;
            read.m_size = kDepthSize;
            graph->AddDispatch("ReadScreenShadowProbe", pipeline, read, Math::CeilDivide(kDepthSize, kThreadGroupSize));
            graph->AddCopyPass(Core::BufferView(readback.Get()), Core::BufferView(output.Get()));

            graph->CompileAndExecute();
            Vulkan::CommandBuffer* commands = Vulkan::ImplCast(queue)->GetCurrentCommandBuffer();
            const Core::FenceSyncPoint fence = queue->CloseFrame();
            commands->EnqueueFenceToSignal(fence);
            commands->Submit();
            fence.Wait();
            device->WaitIdle();

            const auto* values = static_cast<const float*>(readback->Map());
            readback->InvalidateMappedRange(0, byteSize);
            CheckVisibility(light, values);
            readback->Unmap();

            graph.Reset();
        }
    } // namespace


    void Check(Core::Device* device, Core::ResourcePool* pool, Core::GraphicsQueue* queue, Core::DescriptorManager* descriptors,
               Core::PipelineFactory* pipelines)
    {
        Core::CompileGlobalPipelineSets(pipelines);
        Core::WaitForGlobalPipelineSets();

        Core::ComputePipelineRequest request;
        request.m_desc.SetComputeShader("Shaders/Tests/ScreenShadowProbe.cs.hlsl");
        Core::ComputePipeline* pipeline = pipelines->CreateComputePipeline(request);
        pipeline->GetCompletionWaitGroup()->Wait();
        ASSERT_TRUE(pipeline->IsReady());

        // Normalized device coordinates: +x is right and +y is up, so the lights are beyond the right and the top border.
        LightCase right;
        right.m_name = "light on the right";
        right.m_clipPosition = Vector4(4.0f, 0.0f, kFloorDepth, 1.0f);
        right.m_shadowedPixel = Vector2UInt(145, 100);
        right.m_litPixel = Vector2UInt(175, 100);
        right.m_shadowMin = Vector2UInt(86, 86);
        right.m_shadowMax = Vector2UInt(150, 114);
        CheckLight(device, pool, queue, descriptors, pipeline, right);

        LightCase top;
        top.m_name = "light at the top";
        top.m_clipPosition = Vector4(0.0f, 4.0f, kFloorDepth, 1.0f);
        top.m_shadowedPixel = Vector2UInt(160, 115);
        top.m_litPixel = Vector2UInt(160, 85);
        top.m_shadowMin = Vector2UInt(146, 110);
        top.m_shadowMax = Vector2UInt(174, 174);
        CheckLight(device, pool, queue, descriptors, pipeline, top);

        // Behind the camera w is negative: the light projects to the right, while its rays arrive from the left.
        LightCase behind;
        behind.m_name = "light behind the camera";
        behind.m_clipPosition = Vector4(-4.0f, 0.0f, -kFloorDepth, -1.0f);
        behind.m_shadowedPixel = Vector2UInt(175, 100);
        behind.m_litPixel = Vector2UInt(145, 100);
        behind.m_shadowMin = Vector2UInt(170, 86);
        behind.m_shadowMax = Vector2UInt(234, 114);
        CheckLight(device, pool, queue, descriptors, pipeline, behind);
    }
} // namespace FE::Graphics::ScreenShadowProbe
