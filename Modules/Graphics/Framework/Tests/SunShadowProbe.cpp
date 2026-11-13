#include "SunShadowProbe.h"
#include <Graphics/Core/GraphicsQueue.h>
#include <Graphics/Core/PipelineFactory.h>
#include <Graphics/Core/PipelineVariantSet.h>
#include <Graphics/Core/ResourcePool.h>
#include <Graphics/Core/RingUploader.h>
#include <Graphics/Core/Vulkan/GraphicsQueue.h>
#include <Graphics/Features/Shadows/SunShadow.h>
#include <Graphics/Features/Shadows/SunShadowPages.h>
#include <Graphics/Tables/MeshGroupTable.h>
#include <Graphics/Tables/MeshInstanceTable.h>
#include <Shaders/Scene/ViewData.h>
#include <algorithm>
#include <cmath>
#include <gtest/gtest.h>

namespace FE::Graphics::SunShadowProbe
{
    namespace
    {
        constexpr uint32_t kImageSize = 64;
        constexpr uint32_t kBackgroundRows = 16;
        constexpr float kCameraHeight = 10.0f;
        constexpr float kCameraWidth = 8.0f;
        constexpr float kCameraFar = 20.0f;
        //! The y = 0 plane lies at half the depth range of the camera.
        constexpr float kPlaneDepth = (kCameraFar - kCameraHeight) / kCameraFar;


        //! An orthographic camera looking straight down at the y = 0 plane, which lies at half its depth range.
        struct ProbeCamera final
        {
            ViewData m_view{};
            SunShadow::CameraDesc m_desc;
        };


        ProbeCamera CreateCamera(const Vector3 position)
        {
            const Transform transform = Transform::Create(position, Quaternion::RotationX(FE::Constants::kPI * 0.5f), 1.0f);
            const Matrix4x4 projection = Matrix4x4::Orthographic(kCameraWidth, kCameraWidth, 0.0f, kCameraFar);
            const Matrix4x4 viewProjection = Transform::ToMatrix(Math::Invert(transform)) * projection;

            ProbeCamera camera;
            camera.m_view.m_viewProjection = viewProjection;
            camera.m_view.m_inverseViewProjection = Math::Invert(viewProjection);
            camera.m_view.m_cameraPosition = PackedVector4F(Vector4(transform.Translation(), 1.0f));
            camera.m_view.m_viewport = PackedVector4F(0.0f, 0.0f, float(kImageSize), float(kImageSize));
            camera.m_desc.m_position = transform.Translation();
            camera.m_desc.m_pixelFootprint = 2.0f / (projection.m_11 * kImageSize);
            camera.m_desc.m_isOrthographic = true;
            return camera;
        }


        //! The levels stay readable on the host, unlike those that SunShadowData points to.
        struct ProbeShadow final
        {
            SunShadowData m_header{};
            festd::fixed_vector<SunShadowLevel, kSunShadowMaxLevelCount> m_levels;
        };


        Vector2 ProjectTexel(const ProbeShadow& data, const uint32_t level, const Vector3 worldPosition, float& depth)
        {
            const Vector4 clip = Vector4(worldPosition, 1.0f) * data.m_levels[level].m_viewProjection;
            depth = clip.z;
            return Vector2(clip.x * 0.5f + 0.5f, clip.y * -0.5f + 0.5f) * float(kSunShadowVirtualSize);
        }


        // Mirrors SunShadow::GetWorldPage and SunShadow::GetEntryIndex.
        uint32_t GetEntryIndex(const ProbeShadow& data, const uint32_t level, const Vector2Int levelPage)
        {
            const Vector2Int origin = data.m_levels[level].m_pageOrigin;
            const int32_t mask = int32_t(kSunShadowLevelPagesPerAxis - 1);
            const uint32_t slotX = uint32_t((levelPage.x + origin.x) & mask);
            const uint32_t slotY = uint32_t((levelPage.y - origin.y) & mask);
            return level * kSunShadowLevelPageCount + slotY * kSunShadowLevelPagesPerAxis + slotX;
        }


        // Mirrors SunShadow::Project in SunShadow.hlsli and the requests of MarkPages.cs.hlsl, including the toroidal entry
        // addressing of SunShadow::GetPageIndex.
        void RequestPages(const ProbeShadow& data, const Vector3 worldPosition, festd::vector<uint32_t>& entries)
        {
            const float pixelSize = data.m_header.m_cameraPosition.w;
            const uint32_t levelBias =
                uint32_t(Math::Ceil(std::log2(Math::Max(pixelSize / data.m_header.m_level0TexelSize, 1.0f))));
            for (uint32_t level = levelBias; level < data.m_header.m_levelCount; ++level)
            {
                float depth;
                const Vector2 texel = ProjectTexel(data, level, worldPosition, depth);
                const float maxTexel = float(kSunShadowVirtualSize) - 2.0f;
                const bool isInside = texel.x >= 2.0f && texel.y >= 2.0f && texel.x <= maxTexel && texel.y <= maxTexel;
                if (!isInside || depth <= 0.0f || depth >= 1.0f)
                    continue;

                const Vector2Int base(int32_t(Math::Floor(texel.x)), int32_t(Math::Floor(texel.y)));
                for (const Vector2Int offset : { Vector2Int(-3, -3), Vector2Int(3, -3), Vector2Int(-3, 3), Vector2Int(3, 3) })
                {
                    const int32_t x = Math::Clamp(base.x + offset.x, 0, int32_t(kSunShadowVirtualSize - 1));
                    const int32_t y = Math::Clamp(base.y + offset.y, 0, int32_t(kSunShadowVirtualSize - 1));
                    const Vector2Int levelPage(x / int32_t(kSunShadowPageSize), y / int32_t(kSunShadowPageSize));
                    entries.push_back(GetEntryIndex(data, level, levelPage));
                }

                return;
            }
        }


        festd::vector<uint32_t> CalculateExpectedEntries(const ProbeShadow& data, const ViewData& view, const float depth)
        {
            festd::vector<uint32_t> entries;
            for (uint32_t y = kBackgroundRows; y < kImageSize; ++y)
            {
                for (uint32_t x = 0; x < kImageSize; ++x)
                {
                    const Vector2 ndc((x + 0.5f) / kImageSize * 2.0f - 1.0f, (y + 0.5f) / kImageSize * -2.0f + 1.0f);
                    const Vector4 world = Vector4(ndc.x, ndc.y, depth, 1.0f) * view.m_inverseViewProjection;
                    RequestPages(data, Vector3(world.x, world.y, world.z) / world.w, entries);
                }
            }

            std::sort(entries.begin(), entries.end());
            entries.erase(std::unique(entries.begin(), entries.end()), entries.end());
            return entries;
        }


        bool Contains(const festd::vector<uint32_t>& sortedEntries, const uint32_t entry)
        {
            return std::binary_search(sortedEntries.begin(), sortedEntries.end(), entry);
        }


        //! Page table, counters and the first mips of the static and dynamic page pyramids after one frame.
        struct FrameResult final
        {
            festd::vector<uint32_t> m_entries;
            festd::vector<float> m_staticPyramids;
            festd::vector<float> m_dynamicPyramids;
            festd::vector<float> m_staticMips;
            festd::vector<float> m_dynamicMips;
            uint32_t m_counters[kSunShadowCounterCount] = {};
            float m_visibility = 1.0f;
        };


        struct ProbeContext final
        {
            Core::Device* m_device = nullptr;
            Core::ResourcePool* m_pool = nullptr;
            Core::GraphicsQueue* m_queue = nullptr;
            Core::ComputePipeline* m_pipeline = nullptr;
            Core::FrameGraph* m_graph = nullptr;
            Core::RingUploader* m_uploader = nullptr;
            SunShadow::Settings m_settings;
            SunShadow::PageCache m_cache;
        };


        void AddPyramidReadPasses(ProbeContext& context, const SunShadow::PageFrame& frame,
                                  const Core::StructuredBufferPtr<float> output)
        {
            Core::FrameGraph& graph = *context.m_graph;
            for (uint32_t levelIndex = 0; levelIndex < 2 * context.m_settings.m_levelCount; ++levelIndex)
            {
                const uint32_t levelCount = context.m_settings.m_levelCount;
                const bool isStatic = levelIndex < levelCount;
                const Tools::DepthPyramid::Output& pyramid =
                    isStatic ? frame.m_staticPyramids[levelIndex] : frame.m_dynamicPyramids[levelIndex - levelCount];
                ASSERT_EQ(pyramid.m_size, Vector2UInt(kSunShadowLevelPagesPerAxis));

                Constants read{};
                read.m_source = graph.GetSRV(pyramid.m_texture.Get());
                read.m_output = graph.GetUAV(output);
                read.m_size = pyramid.m_size;
                read.m_mode = kReadMode;
                // Static pyramids fill the first page count of texels and dynamic ones the second; local map blocks stay unused.
                const uint32_t level = isStatic ? levelIndex : levelIndex - levelCount;
                read.m_outputOffset = (isStatic ? 0 : frame.m_pageCount) + level * kSunShadowLevelPageCount;
                graph.AddDispatch("ReadSunShadowPagePyramid",
                                  context.m_pipeline,
                                  read,
                                  Math::CeilDivide(pyramid.m_size, kThreadGroupSize));
                uint32_t mipOffset =
                    2 * frame.m_pageCount + (isStatic ? 0 : frame.m_pageCount) + level * kSunShadowLevelPageCount;
                for (uint32_t mip = 1; mip < pyramid.m_mipCount; ++mip)
                {
                    read.m_level = mip;
                    read.m_size = Vector2UInt(Math::Max(pyramid.m_size.x >> mip, 1u), Math::Max(pyramid.m_size.y >> mip, 1u));
                    read.m_outputOffset = mipOffset;
                    graph.AddDispatch("ReadSunShadowPageMip",
                                      context.m_pipeline,
                                      read,
                                      Math::CeilDivide(read.m_size, kThreadGroupSize));
                    mipOffset += read.m_size.x * read.m_size.y;
                }
            }
        }


        struct FrameOptions final
        {
            bool m_requestsCoarse = false;
            bool m_hasDynamicCasters = true;
            bool m_samples = false;
            bool m_writesFeedback = false;
            bool m_fillsAtlas = false;
            float m_depth = kPlaneDepth;
            uint32_t m_fillLevel = 0;
            uint32_t m_requestFirstBlock = kInvalidIndex;
            uint32_t m_requestBlockCount = 1;
            Vector3 m_position = Vector3(3.25f, 5.0f, 3.25f);
        };


        FrameResult RunFrame(ProbeContext& context, const ProbeCamera& camera, const ProbeShadow& data,
                             const festd::span<const SunShadow::LevelSetup> levels, const festd::span<const Aabb> staticChanges,
                             const FrameOptions& options = {})
        {
            Core::Device* device = context.m_device;
            Core::FrameGraph& graph = *context.m_graph;
            context.m_queue->BeginFrame();
            graph.BeginFrame();

            Rc<Core::Texture> depth =
                Core::Texture::Create(device, "SunShadowProbeDepth", Core::Format::kR32_SFLOAT, Vector2UInt(kImageSize));
            Constants fill{};
            fill.m_target = graph.GetUAV(depth.Get());
            fill.m_size = Vector2UInt(kImageSize);
            fill.m_depth = options.m_depth;
            fill.m_backgroundRows = kBackgroundRows;
            fill.m_mode = kFillMode;
            graph.AddDispatch("FillSunShadowProbeDepth",
                              context.m_pipeline,
                              fill,
                              Math::CeilDivide(Vector2UInt(kImageSize), kThreadGroupSize));

            SunShadow::PageInputs inputs;
            SunShadowData header = data.m_header;
            header.m_levels = graph.CreateConstantsArray(festd::span<const SunShadowLevel>(data.m_levels));
            inputs.m_data = graph.CreateConstants(header);
            inputs.m_view = graph.CreateConstants(camera.m_view);
            inputs.m_depth = depth.Get();
            inputs.m_settings = context.m_settings;
            inputs.m_directionToLight = Vector3::AxisY();
            inputs.m_levels = levels;
            inputs.m_staticChanges = staticChanges;
            inputs.m_requestsCoarse = options.m_requestsCoarse;
            const Aabb dynamicBounds(Vector3(-1000.0f), Vector3(1000.0f));
            if (options.m_hasDynamicCasters)
                inputs.m_dynamicBounds = festd::span(&dynamicBounds, 1);
            SunShadow::PageFrame frame;
            SunShadow::AddRequestPasses(graph, *context.m_uploader, inputs, context.m_cache, frame);
            if (options.m_requestFirstBlock != kInvalidIndex)
            {
                Constants request{};
                request.m_mode = kRequestMode;
                request.m_requests = graph.GetUAV(context.m_cache.m_pageTable.Get());
                request.m_size = Vector2UInt(kSunShadowLevelPageCount, options.m_requestBlockCount);
                request.m_outputOffset = options.m_requestFirstBlock * kSunShadowLevelPageCount;
                graph.AddDispatch("RequestProbeBlocks",
                                  context.m_pipeline,
                                  request,
                                  Math::CeilDivide(request.m_size, kThreadGroupSize));
            }

            SunShadow::AddAllocatePasses(graph, context.m_cache, frame);

            auto sample = Core::Buffer::CreateStructured<float>(device, "SunShadowProbeSample", 1);
            if (options.m_fillsAtlas)
            {
                Constants fillAtlas{};
                fillAtlas.m_mode = kFillAtlasMode;
                fillAtlas.m_size = Vector2UInt(kSunShadowPageSize);
                fillAtlas.m_shadow.m_data = inputs.m_data;
                fillAtlas.m_shadow.m_pageTable = graph.GetSRV(context.m_cache.m_pageTable.Get());
                fillAtlas.m_atlas = graph.GetUAV(context.m_cache.m_staticAtlas.Get());
                fillAtlas.m_position = PackedVector3F(options.m_position);
                fillAtlas.m_level = options.m_fillLevel;
                graph.AddDispatch("FillProbeAtlasPage",
                                  context.m_pipeline,
                                  fillAtlas,
                                  Math::CeilDivide(fillAtlas.m_size, kThreadGroupSize));
            }

            if (options.m_samples)
            {
                Constants sampleConstants{};
                sampleConstants.m_mode = kSampleMode;
                sampleConstants.m_size = Vector2UInt(1);
                sampleConstants.m_shadow.m_data = inputs.m_data;
                sampleConstants.m_shadow.m_pageTable = graph.GetSRV(context.m_cache.m_pageTable.Get());
                sampleConstants.m_shadow.m_atlas = graph.GetSRV(frame.m_atlas.Get());
                sampleConstants.m_shadow.m_staticAtlas = graph.GetSRV(context.m_cache.m_staticAtlas.Get());
                if (options.m_writesFeedback)
                    sampleConstants.m_shadow.m_feedback = graph.GetUAV(context.m_cache.m_feedback.Get());

                sampleConstants.m_position = PackedVector3F(options.m_position);
                sampleConstants.m_output = graph.GetUAV(sample.Get());
                graph.AddDispatch("SampleProbeShadow", context.m_pipeline, sampleConstants, 1);
            }

            const uint32_t pageCount = frame.m_pageCount;
            auto pyramidTexels = Core::Buffer::CreateStructured<float>(device, "SunShadowProbePyramids", 4 * pageCount);
            AddPyramidReadPasses(context, frame, pyramidTexels.Get());

            const uint32_t pageBytes = pageCount * sizeof(uint32_t);
            const uint32_t countersOffset = 5 * pageBytes;
            const uint32_t sampleOffset = countersOffset + kSunShadowCounterCount * sizeof(uint32_t);
            const uint32_t byteSize = sampleOffset + sizeof(float);
            Rc<Core::Buffer> readback = Core::Buffer::CreateByteAddress(device, "SunShadowProbeReadback", byteSize);
            Core::ResourceCommitParams readbackCommit;
            readbackCommit.m_memory = Core::ResourceMemory::kHostRandomAccess;
            readbackCommit.m_bindFlags = Core::BarrierAccessFlags::kCopyDest;
            context.m_pool->CommitBufferMemory(readback.Get(), readbackCommit);
            graph.AddCopyPass(Core::BufferView(readback.Get()).Slice(0, pageBytes),
                              Core::BufferView(context.m_cache.m_pageTable.Get()));
            graph.AddCopyPass(Core::BufferView(readback.Get()).Slice(pageBytes, 4 * pageBytes),
                              Core::BufferView(pyramidTexels.Get()));
            graph.AddCopyPass(Core::BufferView(readback.Get()).Slice(countersOffset, kSunShadowCounterCount * sizeof(uint32_t)),
                              Core::BufferView(frame.m_counters.Get()));

            if (options.m_samples)
                graph.AddCopyPass(Core::BufferView(readback.Get()).Slice(sampleOffset, sizeof(float)),
                                  Core::BufferView(sample.Get()));

            graph.CompileAndExecute();
            context.m_uploader->CloseFrame(context.m_queue->GetCurrentFence());
            Vulkan::CommandBuffer* commands = Vulkan::ImplCast(context.m_queue)->GetCurrentCommandBuffer();
            const Core::FenceSyncPoint fence = context.m_queue->CloseFrame();
            commands->EnqueueFenceToSignal(fence);
            commands->Submit();
            fence.Wait();
            device->WaitIdle();

            const auto* words = static_cast<const uint32_t*>(readback->Map());
            readback->InvalidateMappedRange(0, byteSize);
            const auto* pyramidValues = reinterpret_cast<const float*>(words + pageCount);

            FrameResult result;
            result.m_entries.assign(words, words + pageCount);
            result.m_staticPyramids.assign(pyramidValues, pyramidValues + pageCount);
            result.m_dynamicPyramids.assign(pyramidValues + pageCount, pyramidValues + 2 * pageCount);
            result.m_staticMips.assign(pyramidValues + 2 * pageCount, pyramidValues + 3 * pageCount);
            result.m_dynamicMips.assign(pyramidValues + 3 * pageCount, pyramidValues + 4 * pageCount);
            std::copy(words + 5 * pageCount, words + 5 * pageCount + kSunShadowCounterCount, result.m_counters);
            if (options.m_samples)
                memcpy(&result.m_visibility, reinterpret_cast<const uint8_t*>(words) + sampleOffset, sizeof(float));

            readback->Unmap();
            return result;
        }


        //! Pyramid texels are 0 exactly over the entries of the set; texels map to entries through the level origins.
        uint32_t CountPyramidMismatches(const ProbeShadow& data, const festd::vector<float>& pyramids,
                                        const festd::vector<uint32_t>& sortedEntries)
        {
            uint32_t mismatchCount = 0;
            for (uint32_t level = 0; level < data.m_header.m_levelCount; ++level)
            {
                for (uint32_t texel = 0; texel < kSunShadowLevelPageCount; ++texel)
                {
                    const Vector2Int levelPage(int32_t(texel % kSunShadowLevelPagesPerAxis),
                                               int32_t(texel / kSunShadowLevelPagesPerAxis));
                    const bool isDrawn = Contains(sortedEntries, GetEntryIndex(data, level, levelPage));
                    mismatchCount += pyramids[level * kSunShadowLevelPageCount + texel] == (isDrawn ? 0.0f : 1.0f) ? 0 : 1;
                }
            }

            return mismatchCount;
        }


        uint32_t CountMipMismatches(const uint32_t levelCount, const festd::vector<float>& bases,
                                    const festd::vector<float>& mips)
        {
            uint32_t mismatches = 0;
            for (uint32_t level = 0; level < levelCount; ++level)
            {
                festd::array<float, kSunShadowLevelPageCount> values;
                festd::copy_n(bases.data() + level * kSunShadowLevelPageCount, kSunShadowLevelPageCount, values.data());
                uint32_t offset = level * kSunShadowLevelPageCount;
                for (uint32_t size = kSunShadowLevelPagesPerAxis; size > 1; size /= 2)
                {
                    const uint32_t nextSize = size / 2;
                    for (uint32_t index = 0; index < nextSize * nextSize; ++index)
                    {
                        const uint32_t source = index / nextSize * 2 * size + index % nextSize * 2;
                        const float minimum = Math::Min(Math::Min(values[source], values[source + 1]),
                                                        Math::Min(values[source + size], values[source + size + 1]));
                        values[index] = minimum;
                        mismatches += mips[offset + index] == minimum ? 0u : 1u;
                    }

                    offset += nextSize * nextSize;
                }
            }

            return mismatches;
        }


        festd::vector<uint32_t> FindEntries(const FrameResult& result, const uint32_t flags)
        {
            festd::vector<uint32_t> entries;
            for (uint32_t index = 0; index < result.m_entries.size(); ++index)
            {
                if ((result.m_entries[index] & flags) == flags)
                    entries.push_back(index);
            }

            return entries;
        }
    } // namespace


    void Check(Core::Device* device, Core::ResourcePool* pool, Core::GraphicsQueue* queue, Core::DescriptorManager* descriptors,
               Core::PipelineFactory* pipelines)
    {
        Core::CompileGlobalPipelineSets(pipelines);
        Core::WaitForGlobalPipelineSets();

        Core::ComputePipelineRequest request;
        request.m_desc.SetComputeShader("Shaders/Tests/SunShadowProbe.cs.hlsl");
        Core::ComputePipeline* fillPipeline = pipelines->CreateComputePipeline(request);
        fillPipeline->GetCompletionWaitGroup()->Wait();
        ASSERT_TRUE(fillPipeline->IsReady());

        Rc<Core::FrameGraph> graph = device->CreateFrameGraph(descriptors, pool, queue);
        Core::RingUploader uploader;
        uploader.Setup("SunShadowProbeUpload", pool, 64 * 1024);

        ProbeContext context;
        context.m_device = device;
        context.m_pool = pool;
        context.m_queue = queue;
        context.m_pipeline = fillPipeline;
        context.m_graph = graph.Get();
        context.m_uploader = &uploader;

        // The sun shines straight down, so light space is aligned with the world and every pixel projects onto a texel
        // centre of its clip level, away from page boundaries.
        const auto prepare = [&context](const Vector3 cameraPosition,
                                        festd::fixed_vector<SunShadow::LevelSetup, kSunShadowMaxLevelCount>& levels,
                                        ProbeCamera& camera,
                                        ProbeShadow& data) {
            camera = CreateCamera(cameraPosition);
            SunShadow::CalculateLevels(context.m_settings, camera.m_desc.m_position, Vector3::AxisY(), levels);
            data.m_header = SunShadow::BuildShadowData(context.m_settings, camera.m_desc);
            data.m_levels = SunShadow::BuildShadowLevels(levels);
        };

        festd::fixed_vector<SunShadow::LevelSetup, kSunShadowMaxLevelCount> levels;
        ProbeCamera camera;
        ProbeShadow data;
        prepare(Vector3(3.0f, kCameraHeight, 3.0f), levels, camera, data);
        const festd::vector<uint32_t> expected = CalculateExpectedEntries(data, camera.m_view, kPlaneDepth);
        ASSERT_EQ(expected.size(), 4u);

        // A new cache backs every requested page with a distinct atlas page and draws it.
        const FrameResult first = RunFrame(context, camera, data, levels, {});
        EXPECT_EQ(first.m_counters[kSunShadowAllocatedCounter], expected.size());
        EXPECT_EQ(first.m_counters[kSunShadowOverflowCounter], 0u);
        EXPECT_EQ(FindEntries(first, kSunShadowPageAllocated | kSunShadowPageRequested | kSunShadowPageDirty), expected);
        EXPECT_EQ(FindEntries(first, kSunShadowPageAllocated), expected);
        EXPECT_EQ(CountPyramidMismatches(data, first.m_staticPyramids, expected), 0u);
        EXPECT_EQ(CountPyramidMismatches(data, first.m_dynamicPyramids, expected), 0u);
        EXPECT_EQ(CountMipMismatches(context.m_settings.m_levelCount, first.m_staticPyramids, first.m_staticMips), 0u);
        EXPECT_EQ(CountMipMismatches(context.m_settings.m_levelCount, first.m_dynamicPyramids, first.m_dynamicMips), 0u);

        festd::vector<uint32_t> physicalPages;
        for (const uint32_t entry : expected)
            physicalPages.push_back(first.m_entries[entry] & kSunShadowPhysicalPageMask);
        std::sort(physicalPages.begin(), physicalPages.end());
        EXPECT_EQ(std::unique(physicalPages.begin(), physicalPages.end()), physicalPages.end());

        // The same view keeps its pages: nothing is drawn into the static atlas, and the dynamic casters cover the same pages.
        const FrameResult second = RunFrame(context, camera, data, levels, {});
        EXPECT_EQ(second.m_counters[kSunShadowAllocatedCounter], 0u);
        EXPECT_TRUE(FindEntries(second, kSunShadowPageDirty).empty());
        EXPECT_EQ(FindEntries(second, kSunShadowPageAllocated | kSunShadowPageRequested), expected);
        for (const uint32_t entry : expected)
            EXPECT_EQ(second.m_entries[entry] & kSunShadowPhysicalPageMask, first.m_entries[entry] & kSunShadowPhysicalPageMask);
        EXPECT_EQ(CountPyramidMismatches(data, second.m_staticPyramids, {}), 0u);
        EXPECT_EQ(CountPyramidMismatches(data, second.m_dynamicPyramids, expected), 0u);
        EXPECT_EQ(CountMipMismatches(context.m_settings.m_levelCount, second.m_staticPyramids, second.m_staticMips), 0u);
        EXPECT_EQ(CountMipMismatches(context.m_settings.m_levelCount, second.m_dynamicPyramids, second.m_dynamicMips), 0u);

        // A static change under the centre of the view discards the one page it covers, which is drawn again.
        const Aabb change(Vector3(2.9f, -0.1f, 2.9f), Vector3(3.1f, 0.1f, 3.1f));
        const FrameResult third = RunFrame(context, camera, data, levels, festd::span(&change, 1));
        EXPECT_EQ(third.m_counters[kSunShadowAllocatedCounter], 1u);
        EXPECT_EQ(FindEntries(third, kSunShadowPageDirty).size(), 1u);
        EXPECT_EQ(FindEntries(third, kSunShadowPageAllocated | kSunShadowPageRequested), expected);

        // Moving the camera by one page of its level keeps the pages that are still requested and draws only the new ones.
        prepare(Vector3(3.0f + 16.0f, kCameraHeight, 3.0f), levels, camera, data);
        const festd::vector<uint32_t> moved = CalculateExpectedEntries(data, camera.m_view, kPlaneDepth);
        ASSERT_EQ(moved.size(), 4u);
        const FrameResult fourth = RunFrame(context, camera, data, levels, {});
        uint32_t keptCount = 0;
        uint32_t mismatchCount = 0;
        for (const uint32_t entry : moved)
        {
            const bool isKept = Contains(expected, entry);
            keptCount += isKept ? 1 : 0;
            const uint32_t value = fourth.m_entries[entry];
            const bool isDirty = (value & kSunShadowPageDirty) != 0;
            const bool isSamePage = (value & kSunShadowPhysicalPageMask) == (third.m_entries[entry] & kSunShadowPhysicalPageMask);
            mismatchCount += isKept ? (isDirty || !isSamePage ? 1 : 0) : (isDirty ? 0 : 1);
        }

        EXPECT_EQ(keptCount, 2u);
        EXPECT_EQ(mismatchCount, 0u);
        EXPECT_EQ(fourth.m_counters[kSunShadowAllocatedCounter], moved.size() - keptCount);
        EXPECT_EQ(fourth.m_counters[kSunShadowOverflowCounter], 0u);
        EXPECT_EQ(FindEntries(fourth, kSunShadowPageAllocated | kSunShadowPageRequested), moved);

        // No dynamic overlap means sampling the static atlas directly, with an entirely occluded dynamic mask.
        FrameOptions staticOnly;
        staticOnly.m_hasDynamicCasters = false;
        const FrameResult noDynamic = RunFrame(context, camera, data, levels, {}, staticOnly);
        EXPECT_TRUE(FindEntries(noDynamic, kSunShadowPageDynamic).empty());
        EXPECT_EQ(CountPyramidMismatches(data, noDynamic.m_dynamicPyramids, {}), 0u);

        // Coarse requests cover transparent receivers even with no opaque depth. Sampling emits fine feedback for the next frame.
        context.m_cache = {};
        prepare(Vector3(3.0f, kCameraHeight, 3.0f), levels, camera, data);
        FrameOptions transparent;
        transparent.m_depth = 0.0f;
        transparent.m_hasDynamicCasters = false;
        transparent.m_requestsCoarse = true;
        transparent.m_samples = true;
        transparent.m_writesFeedback = true;
        transparent.m_fillsAtlas = true;
        transparent.m_fillLevel = context.m_settings.m_levelCount - 1;
        const FrameResult coarse = RunFrame(context, camera, data, levels, {}, transparent);
        EXPECT_FALSE(FindEntries(coarse, kSunShadowPageAllocated | kSunShadowPageCoarse).empty());
        EXPECT_FLOAT_EQ(coarse.m_visibility, 0.0f);

        festd::vector<uint32_t> feedbackEntries;
        RequestPages(data, transparent.m_position, feedbackEntries);
        std::sort(feedbackEntries.begin(), feedbackEntries.end());
        feedbackEntries.erase(std::unique(feedbackEntries.begin(), feedbackEntries.end()), feedbackEntries.end());
        transparent.m_requestsCoarse = false;
        transparent.m_writesFeedback = false;
        transparent.m_fillsAtlas = false;
        const FrameResult feedback = RunFrame(context, camera, data, levels, {}, transparent);
        EXPECT_EQ(FindEntries(feedback, kSunShadowPageAllocated | kSunShadowPageRequested), feedbackEntries);
        EXPECT_FLOAT_EQ(feedback.m_visibility, 1.0f);
        const FrameResult consumed = RunFrame(context, camera, data, levels, {}, transparent);
        EXPECT_TRUE(FindEntries(consumed, kSunShadowPageRequested).empty());

        // The debug override ignores queued fine feedback and covers the whole view without transparent bounds.
        transparent.m_writesFeedback = true;
        RunFrame(context, camera, data, levels, {}, transparent);
        context.m_settings.m_forceCoarsePages = true;
        const FrameResult coarseOnly = RunFrame(context, camera, data, levels, {}, transparent);
        const auto coarseRequests = FindEntries(coarseOnly, kSunShadowPageRequested);
        EXPECT_FALSE(coarseRequests.empty());
        EXPECT_EQ(FindEntries(coarseOnly, kSunShadowPageRequested | kSunShadowPageCoarse), coarseRequests);

        const FrameResult coarseOpaque = RunFrame(context, camera, data, levels, {});
        const auto opaqueCoarseRequests = FindEntries(coarseOpaque, kSunShadowPageRequested);
        EXPECT_FALSE(opaqueCoarseRequests.empty());
        EXPECT_EQ(FindEntries(coarseOpaque, kSunShadowPageRequested | kSunShadowPageCoarse), opaqueCoarseRequests);

        context.m_settings.m_forceCoarsePages = false;
        const FrameResult fineAgain = RunFrame(context, camera, data, levels, {});
        EXPECT_FALSE(FindEntries(fineAgain, kSunShadowPageRequested).empty());
        EXPECT_TRUE(FindEntries(fineAgain, kSunShadowPageRequested | kSunShadowPageCoarse).empty());

        // A scroll discards feedback for toroidal slots that now describe different world pages.
        transparent.m_writesFeedback = true;
        RunFrame(context, camera, data, levels, {}, transparent);
        prepare(Vector3(100000.0f, kCameraHeight, 3.0f), levels, camera, data);
        transparent.m_writesFeedback = false;
        transparent.m_samples = false;
        const FrameResult scrolled = RunFrame(context, camera, data, levels, {}, transparent);
        EXPECT_TRUE(FindEntries(scrolled, kSunShadowPageRequested).empty());

        // Existing far residency cannot defeat new near requests when the atlas overflows.
        context.m_cache = {};
        context.m_settings.m_levelCount = 2;
        prepare(Vector3(3.0f, kCameraHeight, 3.0f), levels, camera, data);
        FrameOptions pressure;
        pressure.m_depth = 0.0f;
        pressure.m_hasDynamicCasters = false;
        pressure.m_requestFirstBlock = 1;
        const FrameResult far = RunFrame(context, camera, data, levels, {}, pressure);
        EXPECT_EQ(far.m_counters[kSunShadowAllocatedCounter], kSunShadowPhysicalPageCount);
        pressure.m_requestFirstBlock = 0;
        pressure.m_requestBlockCount = 2;
        const FrameResult overflow = RunFrame(context, camera, data, levels, {}, pressure);
        const auto admitted = FindEntries(overflow, kSunShadowPageAllocated);
        ASSERT_EQ(admitted.size(), kSunShadowPhysicalPageCount);
        EXPECT_LT(admitted.back(), kSunShadowLevelPageCount);
        EXPECT_EQ(overflow.m_counters[kSunShadowOverflowCounter], kSunShadowLevelPageCount);

        context.m_cache = {};
        graph.Reset();
        uploader.Shutdown();
    }
} // namespace FE::Graphics::SunShadowProbe


TEST(SunShadow, ClipLevelsSnapToWholePages)
{
    using namespace FE;
    using namespace FE::Graphics;

    const SunShadow::Settings settings;
    const Vector3 directionToLight = Math::Normalize(Vector3(0.3f, 1.0f, 0.2f));
    const Vector3 cameraPosition(13.7f, 2.5f, -41.3f);
    festd::fixed_vector<SunShadow::LevelSetup, kSunShadowMaxLevelCount> levels;
    SunShadow::CalculateLevels(settings, cameraPosition, directionToLight, levels);
    ASSERT_EQ(levels.size(), settings.m_levelCount);

    for (uint32_t levelIndex = 0; levelIndex < levels.size(); ++levelIndex)
    {
        const SunShadow::LevelSetup& level = levels[levelIndex];
        EXPECT_FLOAT_EQ(level.m_size, settings.m_level0Size * float(1u << levelIndex));
        EXPECT_FLOAT_EQ(level.m_texelSize, level.m_size / kSunShadowVirtualSize);

        // The level looks along the light and keeps the camera in the middle half of its depth range, which moves in quarters.
        const Vector3 forward = Math::Rotate(Vector3::AxisZ(), level.m_camera.Rotation());
        EXPECT_TRUE(Math::CmpEqual(forward, -directionToLight, 1e-4f));
        EXPECT_EQ(level.m_depthOrigin, levels[0].m_depthOrigin);

        const Vector4 clip = Vector4(cameraPosition, 1.0f) * level.m_viewProjection;
        EXPECT_GT(clip.z, 0.25f - 1e-4f);
        EXPECT_LE(clip.z, 0.75f + 1e-4f);

        // Snapping moves the level by less than a page, in whole pages, so the camera stays in the centre pages.
        const float texelX = (clip.x * 0.5f + 0.5f) * kSunShadowVirtualSize;
        const float texelY = (clip.y * -0.5f + 0.5f) * kSunShadowVirtualSize;
        const float centre = kSunShadowVirtualSize * 0.5f;
        EXPECT_GE(texelX, centre - 1e-2f);
        EXPECT_LT(texelX, centre + kSunShadowPageSize + 1e-2f);
        EXPECT_GE(texelY, centre - kSunShadowPageSize - 1e-2f);
        EXPECT_LT(texelY, centre + 1e-2f);
    }

    // Moving the camera within a page of the coarsest level keeps that level in place; moving it by a whole page moves the
    // level origin by one page.
    festd::fixed_vector<SunShadow::LevelSetup, kSunShadowMaxLevelCount> moved;
    const Vector3 lightRight = Math::Rotate(Vector3::AxisX(), levels.back().m_camera.Rotation());
    SunShadow::CalculateLevels(settings, cameraPosition + lightRight * 1e-3f, directionToLight, moved);
    EXPECT_TRUE(Math::CmpEqual(moved.back().m_camera.Translation(), levels.back().m_camera.Translation(), 1e-3f));

    const float pageSize = levels.back().m_size / kSunShadowLevelPagesPerAxis;
    SunShadow::CalculateLevels(settings, cameraPosition + lightRight * pageSize, directionToLight, moved);
    EXPECT_EQ(moved.back().m_pageOrigin, levels.back().m_pageOrigin + Vector2Int(1, 0));
}


TEST(SunShadow, DepthOriginRemainsStableAcrossOrdinaryCameraMotion)
{
    using namespace FE;
    using namespace FE::Graphics;

    const SunShadow::Settings settings;
    festd::fixed_vector<SunShadow::LevelSetup, kSunShadowMaxLevelCount> first;
    festd::fixed_vector<SunShadow::LevelSetup, kSunShadowMaxLevelCount> moved;
    SunShadow::CalculateLevels(settings, Vector3(0.0f, 1.0f, 0.0f), Vector3::AxisY(), first);
    const float origin = first[0].m_depthOrigin;
    SunShadow::CalculateLevels(settings, Vector3(0.0f, -300.0f, 0.0f), Vector3::AxisY(), moved, &origin);
    EXPECT_FLOAT_EQ(moved[0].m_depthOrigin, origin);
}
