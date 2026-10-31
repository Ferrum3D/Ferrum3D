#include "SunShadowProbe.h"
#include <Graphics/Core/GraphicsQueue.h>
#include <Graphics/Core/PipelineFactory.h>
#include <Graphics/Core/PipelineVariantSet.h>
#include <Graphics/Core/ResourcePool.h>
#include <Graphics/Core/RingUploader.h>
#include <Graphics/Core/Vulkan/GraphicsQueue.h>
#include <Graphics/Features/Shadows/SunShadow.h>
#include <Graphics/Features/Shadows/SunShadowPages.h>
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


        //! An orthographic camera looking straight down at the y = 0 plane, which lies at half its depth range.
        struct ProbeCamera final
        {
            ViewData m_view{};
            SunShadow::CameraDesc m_desc;
        };


        ProbeCamera CreateCamera()
        {
            const Transform transform =
                Transform::Create(Vector3(3.0f, kCameraHeight, 3.0f), Quaternion::RotationX(FE::Constants::kPI * 0.5f), 1.0f);
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


        Vector2 ProjectTexel(const SunShadowData& data, const uint32_t level, const Vector3 worldPosition, float& depth)
        {
            const Vector4 clip = Vector4(worldPosition, 1.0f) * data.m_levels[level].m_viewProjection;
            depth = clip.z;
            return Vector2(clip.x * 0.5f + 0.5f, clip.y * -0.5f + 0.5f) * float(kSunShadowVirtualSize);
        }


        // Mirrors SunShadow::Project in SunShadow.hlsli and the requests of MarkPages.cs.hlsl.
        void RequestPages(const SunShadowData& data, const Vector3 worldPosition, festd::vector<uint32_t>& pages)
        {
            const float pixelSize = data.m_cameraPosition.w;
            const uint32_t levelBias = uint32_t(Math::Floor(std::log2(Math::Max(pixelSize / data.m_level0TexelSize, 1.0f))));
            for (uint32_t level = levelBias; level < data.m_levelCount; ++level)
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
                    const uint32_t x = uint32_t(Math::Clamp(base.x + offset.x, 0, int32_t(kSunShadowVirtualSize - 1)));
                    const uint32_t y = uint32_t(Math::Clamp(base.y + offset.y, 0, int32_t(kSunShadowVirtualSize - 1)));
                    const uint32_t page = (y / kSunShadowPageSize) * kSunShadowLevelPagesPerAxis + x / kSunShadowPageSize;
                    pages.push_back(level * kSunShadowLevelPageCount + page);
                }

                return;
            }
        }


        festd::vector<uint32_t> CalculateExpectedPages(const SunShadowData& data, const ViewData& view, const float depth)
        {
            festd::vector<uint32_t> pages;
            for (uint32_t y = kBackgroundRows; y < kImageSize; ++y)
            {
                for (uint32_t x = 0; x < kImageSize; ++x)
                {
                    const Vector2 ndc((x + 0.5f) / kImageSize * 2.0f - 1.0f, (y + 0.5f) / kImageSize * -2.0f + 1.0f);
                    const Vector4 world = Vector4(ndc.x, ndc.y, depth, 1.0f) * view.m_inverseViewProjection;
                    RequestPages(data, Vector3(world.x, world.y, world.z) / world.w, pages);
                }
            }

            std::sort(pages.begin(), pages.end());
            pages.erase(std::unique(pages.begin(), pages.end()), pages.end());
            return pages;
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

        // The sun shines straight down, so light space is aligned with the world and every pixel projects onto a texel
        // centre of its clip level, away from page boundaries.
        const SunShadow::Settings settings;
        const ProbeCamera camera = CreateCamera();
        festd::fixed_vector<SunShadow::LevelSetup, kSunShadowMaxLevelCount> levels;
        SunShadow::CalculateLevels(settings, camera.m_desc.m_position, Vector3::AxisY(), levels);
        const SunShadowData data = SunShadow::BuildShadowData(settings, levels, camera.m_desc);

        const float planeDepth = (kCameraFar - kCameraHeight) / kCameraFar;
        const festd::vector<uint32_t> expectedPages = CalculateExpectedPages(data, camera.m_view, planeDepth);
        ASSERT_EQ(expectedPages.size(), 4u);

        Rc<Core::FrameGraph> graph = device->CreateFrameGraph(descriptors, pool, queue);
        Core::RingUploader uploader;
        uploader.Setup("SunShadowProbeUpload", pool, 64 * 1024);
        queue->BeginFrame();
        graph->BeginFrame();

        Rc<Core::Texture> depth =
            Core::Texture::Create(device, "SunShadowProbeDepth", Core::Format::kR32_SFLOAT, Vector2UInt(kImageSize));
        auto* fill = graph->AllocatePassData<PassDesc>();
        fill->m_pipeline = fillPipeline;
        fill->m_constants.m_target = graph->GetUAV(depth.Get());
        fill->m_constants.m_size = Vector2UInt(kImageSize);
        fill->m_constants.m_depth = planeDepth;
        fill->m_constants.m_backgroundRows = kBackgroundRows;
        graph->AddDispatchPass("FillSunShadowProbeDepth", fill, Math::CeilDivide(Vector2UInt(kImageSize), kThreadGroupSize));

        Rc<Core::Buffer> viewBuffer = Core::Buffer::CreateStructured<ViewData>(device, "SunShadowProbeView", 1);
        Rc<Core::Buffer> dataBuffer = Core::Buffer::CreateStructured<SunShadowData>(device, "SunShadowProbeData", 1);
        FE_Verify(uploader.Upload(*graph, viewBuffer.Get(), camera.m_view));
        FE_Verify(uploader.Upload(*graph, dataBuffer.Get(), data));

        const SunShadow::PageResources resources = SunShadow::PageResources::Create(device, settings.m_levelCount);
        SunShadow::AddPagePasses(*graph, { dataBuffer.Get(), viewBuffer.Get(), depth.Get() }, resources);

        const uint32_t pageBytes = resources.m_pageCount * sizeof(uint32_t);
        const uint32_t byteSize = pageBytes + kSunShadowCounterCount * sizeof(uint32_t);
        Rc<Core::Buffer> readback = Core::Buffer::CreateByteAddress(device, "SunShadowProbeReadback", byteSize);
        Core::ResourceCommitParams readbackCommit;
        readbackCommit.m_memory = Core::ResourceMemory::kHostRandomAccess;
        readbackCommit.m_bindFlags = Core::BarrierAccessFlags::kCopyDest;
        pool->CommitBufferMemory(readback.Get(), readbackCommit);
        graph->AddCopyPass(Core::BufferView(readback.Get()).Slice(0, pageBytes), Core::BufferView(resources.m_pageTable.Get()));
        graph->AddCopyPass(Core::BufferView(readback.Get()).Slice(pageBytes, kSunShadowCounterCount * sizeof(uint32_t)),
                           Core::BufferView(resources.m_counters.Get()));

        graph->CompileAndExecute();
        uploader.CloseFrame(queue->GetCurrentFence());
        Vulkan::CommandBuffer* commands = Vulkan::ImplCast(queue)->GetCurrentCommandBuffer();
        const Core::FenceSyncPoint fence = queue->CloseFrame();
        commands->EnqueueFenceToSignal(fence);
        commands->Submit();
        fence.Wait();
        device->WaitIdle();

        const auto* words = static_cast<const uint32_t*>(readback->Map());
        readback->InvalidateMappedRange(0, byteSize);
        const uint32_t* counters = words + resources.m_pageCount;
        EXPECT_EQ(counters[kSunShadowAllocatedCounter], expectedPages.size());
        EXPECT_EQ(counters[kSunShadowOverflowCounter], 0u);

        festd::vector<bool> isPhysicalPageUsed(kSunShadowPhysicalPageCount, false);
        uint32_t mismatchCount = 0;
        for (uint32_t pageIndex = 0; pageIndex < resources.m_pageCount; ++pageIndex)
        {
            const uint32_t entry = words[pageIndex];
            const bool isExpected = std::binary_search(expectedPages.begin(), expectedPages.end(), pageIndex);
            if (!isExpected)
            {
                mismatchCount += entry != 0 ? 1 : 0;
                continue;
            }

            const uint32_t physicalPage = entry & kSunShadowPhysicalPageMask;
            const bool isBacked = (entry & kSunShadowPageRequested) != 0 && (entry & kSunShadowPageAllocated) != 0;
            const bool isUnique = physicalPage < expectedPages.size() && !isPhysicalPageUsed[physicalPage];
            mismatchCount += isBacked && isUnique ? 0 : 1;
            if (isUnique)
                isPhysicalPageUsed[physicalPage] = true;
        }

        EXPECT_EQ(mismatchCount, 0u);
        readback->Unmap();
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

        // The level looks along the light and keeps the camera in the middle of its depth range.
        const Vector3 forward = Math::Rotate(Vector3::AxisZ(), level.m_camera.Rotation());
        EXPECT_TRUE(Math::CmpEqual(forward, -directionToLight, 1e-4f));

        const Vector4 clip = Vector4(cameraPosition, 1.0f) * level.m_viewProjection;
        EXPECT_NEAR(clip.z, 0.5f, 1e-4f);

        // Snapping moves the level by less than a page, in whole pages, so the camera stays in the centre pages.
        const float texelX = (clip.x * 0.5f + 0.5f) * kSunShadowVirtualSize;
        const float texelY = (clip.y * -0.5f + 0.5f) * kSunShadowVirtualSize;
        const float centre = kSunShadowVirtualSize * 0.5f;
        EXPECT_GE(texelX, centre - 1e-2f);
        EXPECT_LT(texelX, centre + kSunShadowPageSize + 1e-2f);
        EXPECT_GE(texelY, centre - kSunShadowPageSize - 1e-2f);
        EXPECT_LT(texelY, centre + 1e-2f);
    }

    // Moving the camera within a page of the coarsest level keeps that level in place.
    festd::fixed_vector<SunShadow::LevelSetup, kSunShadowMaxLevelCount> moved;
    const Vector3 lightRight = Math::Rotate(Vector3::AxisX(), levels.back().m_camera.Rotation());
    SunShadow::CalculateLevels(settings, cameraPosition + lightRight * 1e-3f, directionToLight, moved);
    EXPECT_TRUE(Math::CmpEqual(moved.back().m_camera.Translation(), levels.back().m_camera.Translation(), 1e-3f));
}
