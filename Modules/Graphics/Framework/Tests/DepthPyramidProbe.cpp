#include "DepthPyramidProbe.h"
#include <Graphics/Core/DescriptorManager.h>
#include <Graphics/Core/GraphicsQueue.h>
#include <Graphics/Core/PipelineFactory.h>
#include <Graphics/Core/PipelineVariantSet.h>
#include <Graphics/Core/ResourcePool.h>
#include <Graphics/Core/Vulkan/GraphicsQueue.h>
#include <Graphics/Passes/Tools/DepthPyramid.h>
#include <gtest/gtest.h>

namespace FE::Graphics::DepthPyramidProbe
{
    namespace
    {
        using Reducer = float (*)(float, float);


        float ReduceFarthest(const float lhs, const float rhs)
        {
            return Math::Min(lhs, rhs);
        }


        float ReduceClosest(const float lhs, const float rhs)
        {
            return Math::Max(lhs, rhs);
        }


        // Every texel reduces the 2x2 texels of the level above; coordinates beyond a level repeat its edge.
        festd::vector<float> ReduceLevel(const festd::vector<float>& source, const Vector2UInt sourceSize, const Vector2UInt size,
                                         const Reducer reduce)
        {
            festd::vector<float> result(size.x * size.y);
            for (uint32_t y = 0; y < size.y; ++y)
            {
                for (uint32_t x = 0; x < size.x; ++x)
                {
                    float value = source[Math::Min(2 * y, sourceSize.y - 1) * sourceSize.x + Math::Min(2 * x, sourceSize.x - 1)];
                    for (uint32_t offset = 1; offset < 4; ++offset)
                    {
                        const uint32_t sourceX = Math::Min(2 * x + offset % 2, sourceSize.x - 1);
                        const uint32_t sourceY = Math::Min(2 * y + offset / 2, sourceSize.y - 1);
                        value = reduce(value, source[sourceY * sourceSize.x + sourceX]);
                    }

                    result[y * size.x + x] = value;
                }
            }

            return result;
        }


        uint32_t CountMismatches(const Tools::DepthPyramid::Output& pyramid, const Vector2UInt depthSize, const Reducer reduce,
                                 const float* gpuValues)
        {
            festd::vector<float> level(depthSize.x * depthSize.y);
            for (uint32_t y = 0; y < depthSize.y; ++y)
            {
                for (uint32_t x = 0; x < depthSize.x; ++x)
                    level[y * depthSize.x + x] = HashDepth(x, y);
            }

            uint32_t mismatches = 0;
            uint32_t offset = 0;
            Vector2UInt levelSize = depthSize;
            for (uint32_t mipIndex = 0; mipIndex < pyramid.m_mipCount; ++mipIndex)
            {
                const Vector2UInt mipSize(Math::Max(pyramid.m_size.x >> mipIndex, 1u),
                                          Math::Max(pyramid.m_size.y >> mipIndex, 1u));
                level = ReduceLevel(level, levelSize, mipSize, reduce);
                levelSize = mipSize;
                for (uint32_t texel = 0; texel < mipSize.x * mipSize.y; ++texel)
                {
                    if (gpuValues[offset + texel] == level[texel])
                        continue;

                    if (mismatches < 8)
                    {
                        ADD_FAILURE() << "mip " << mipIndex << " texel (" << texel % mipSize.x << ", " << texel / mipSize.x
                                      << "): expected " << level[texel] << ", got " << gpuValues[offset + texel];
                    }

                    ++mismatches;
                }

                offset += mipSize.x * mipSize.y;
            }

            return mismatches;
        }


        uint32_t CalculateTexelCount(const Tools::DepthPyramid::Output& pyramid)
        {
            uint32_t count = 0;
            for (uint32_t mipIndex = 0; mipIndex < pyramid.m_mipCount; ++mipIndex)
                count += Math::Max(pyramid.m_size.x >> mipIndex, 1u) * Math::Max(pyramid.m_size.y >> mipIndex, 1u);

            return count;
        }


        void AddReadPasses(Core::FrameGraph& graph, Core::ComputePipeline* pipeline, const Tools::DepthPyramid::Output& pyramid,
                           Core::Buffer* output, uint32_t& offset)
        {
            for (uint32_t mipIndex = 0; mipIndex < pyramid.m_mipCount; ++mipIndex)
            {
                const Vector2UInt mipSize(Math::Max(pyramid.m_size.x >> mipIndex, 1u),
                                          Math::Max(pyramid.m_size.y >> mipIndex, 1u));
                const auto subresource = Core::TextureSubresource::Create(pyramid.m_texture->GetDesc(), mipIndex, 0);

                auto* read = graph.AllocatePassData<PassDesc>();
                read->m_pipeline = pipeline;
                read->m_constants.m_source = graph.GetSRV(Core::TextureView::Create(pyramid.m_texture.Get(), subresource));
                read->m_constants.m_output = graph.GetUAV(output);
                read->m_constants.m_mode = kReadMode;
                read->m_constants.m_size = mipSize;
                read->m_constants.m_outputOffset = offset;
                graph.AddDispatchPass("ReadDepthPyramidMip", read, Math::CeilDivide(mipSize, kThreadGroupSize));
                offset += mipSize.x * mipSize.y;
            }
        }


        void CheckSize(Core::Device* device, Core::ResourcePool* pool, Core::GraphicsQueue* queue,
                       Core::DescriptorManager* descriptors, Core::ComputePipeline* pipeline, const Vector2UInt depthSize)
        {
            SCOPED_TRACE(Fmt::Format("{}x{}", depthSize.x, depthSize.y).c_str());

            Rc<Core::FrameGraph> graph = device->CreateFrameGraph(descriptors, pool, queue);

            queue->BeginFrame();
            graph->BeginFrame();

            Rc<Core::Texture> depth =
                Core::Texture::Create(device, "DepthPyramidProbeDepth", Core::Format::kR32_SFLOAT, depthSize);
            auto* fill = graph->AllocatePassData<PassDesc>();
            fill->m_pipeline = pipeline;
            fill->m_constants.m_target = graph->GetUAV(depth.Get());
            fill->m_constants.m_mode = kFillMode;
            fill->m_constants.m_size = depthSize;
            graph->AddDispatchPass("FillDepthPyramidProbe", fill, Math::CeilDivide(depthSize, kThreadGroupSize));

            const auto pyramids = Tools::DepthPyramid::AddPass(*graph, depth.Get());
            const uint32_t farthestCount = CalculateTexelCount(pyramids.m_farthest);
            const uint32_t byteSize = 2 * farthestCount * sizeof(float);
            Rc<Core::Buffer> output = Core::Buffer::CreateByteAddress(device, "DepthPyramidProbeOutput", byteSize);
            Rc<Core::Buffer> readback = Core::Buffer::CreateByteAddress(device, "DepthPyramidProbeReadback", byteSize);
            Core::ResourceCommitParams readbackCommit;
            readbackCommit.m_memory = Core::ResourceMemory::kHostRandomAccess;
            readbackCommit.m_bindFlags = Core::BarrierAccessFlags::kCopyDest;
            pool->CommitBufferMemory(readback.Get(), readbackCommit);

            uint32_t offset = 0;
            AddReadPasses(*graph, pipeline, pyramids.m_farthest, output.Get(), offset);
            AddReadPasses(*graph, pipeline, pyramids.m_closest, output.Get(), offset);
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
            EXPECT_EQ(CountMismatches(pyramids.m_farthest, depthSize, ReduceFarthest, values), 0u);
            EXPECT_EQ(CountMismatches(pyramids.m_closest, depthSize, ReduceClosest, values + farthestCount), 0u);
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
        request.m_desc.SetComputeShader("Shaders/Tests/DepthPyramidProbe.cs.hlsl");
        Core::ComputePipeline* pipeline = pipelines->CreateComputePipeline(request);
        pipeline->GetCompletionWaitGroup()->Wait();
        ASSERT_TRUE(pipeline->IsReady());

        // A small target only needs the tile pass and one downsampler mip; the wide one also exercises the last group of the
        // downsampler, which reduces the mid mip, and non-square mips that reach a width of one texel.
        CheckSize(device, pool, queue, descriptors, pipeline, Vector2UInt(37, 23));
        CheckSize(device, pool, queue, descriptors, pipeline, Vector2UInt(2100, 300));
    }
} // namespace FE::Graphics::DepthPyramidProbe
