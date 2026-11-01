#include <Graphics/Core/PipelineVariantSet.h>
#include <Graphics/Core/ResourcePool.h>
#include <Graphics/Features/Lighting/GlobalLightingSceneModule.h>
#include <Graphics/Features/Mesh/MeshSceneModule.h>
#include <Graphics/Features/Shadows/SunShadow.h>
#include <Graphics/Features/Shadows/SunShadowPages.h>
#include <Graphics/Passes/DepthPyramidPass.h>
#include <Graphics/Passes/DrawTags.h>
#include <Graphics/Passes/ForwardTransparencyPass.h>
#include <Graphics/Passes/RendererPassCommon.h>
#include <Graphics/Passes/SurfaceEvaluationPass.h>
#include <Graphics/Passes/VisibilityPass.h>
#include <Graphics/Renderer.h>
#include <Graphics/Scene/Culling.h>
#include <Graphics/Scene/Scene.h>

namespace FE::Graphics::SunShadow
{
    namespace
    {
        FE_COMPUTE_PIPELINE_SET(UpdatePagesPipeline, "Shaders/Shadows/UpdatePages.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(InvalidatePagesPipeline, "Shaders/Shadows/InvalidatePages.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(MarkPagesPipeline, "Shaders/Shadows/MarkPages.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(CollectPagesPipeline, "Shaders/Shadows/CollectPages.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(AllocatePagesPipeline, "Shaders/Shadows/AllocatePages.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(ClearAtlasPagesPipeline, "Shaders/Shadows/ClearAtlasPages.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(CopyPagesPipeline, "Shaders/Shadows/CopyPages.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(BuildPageMaskPipeline, "Shaders/Shadows/BuildPageMask.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(SunShadowDebugPipeline, "Shaders/Shadows/SunShadowDebug.cs.hlsl");

        constexpr uint32_t kDebugModes[] = { kSunShadowDebugLevel,
                                             kSunShadowDebugPage,
                                             kSunShadowDebugFactor,
                                             kSunShadowDebugLocalFactor };

        //! Pages whose static casters are drawn this frame, and pages that are sampled this frame.
        constexpr uint32_t kStaticPageFlags = kSunShadowPageAllocated | kSunShadowPageDirty;
        constexpr uint32_t kDynamicPageFlags = kSunShadowPageAllocated | kSunShadowPageRequested;


        struct DebugPassData final
        {
            uint32_t m_mode = kSunShadowDebugLevel;
        };


        festd::span<const Env::Name> GetModeNames()
        {
            // Names are interned on first use, after the environment is initialized.
            static const Env::Name kModeNames[] = { Env::Name("shadow-level"),
                                                    Env::Name("shadow-page"),
                                                    Env::Name("shadow-factor"),
                                                    Env::Name("local-shadow") };
            return kModeNames;
        }


        // A view basis looking along the light, i.e. towards -directionToLight.
        Quaternion CalculateLightRotation(const Vector3 directionToLight)
        {
            const Vector3 forward = -directionToLight;
            const Vector3 up = Math::Abs(forward.y) > 0.99f ? Vector3::AxisX() : Vector3::AxisY();
            return Math::ExtractRotation(Math::Invert(Matrix4x4::LookAt(Vector3::kZero, forward, up)));
        }


        Vector3 GetDirectionToLight(const GlobalLightingParameters& parameters)
        {
            return Math::Rotate(Vector3::AxisZ(), Math::Normalize(parameters.m_directionalLightOrientation));
        }


        Rc<Core::Buffer> CreatePersistentBuffer(Core::FrameGraph& graph, const Env::Name name, const uint32_t count)
        {
            Rc<Core::Buffer> buffer = Core::Buffer::CreateStructured<uint32_t>(graph.GetDevice(), name, count);
            Core::ResourceCommitParams commitParams;
            commitParams.m_memory = Core::ResourceMemory::kDeviceLocal;
            // Copies read the cache back for diagnostics and tests.
            commitParams.m_bindFlags = Core::BarrierAccessFlags::kShaderRead | Core::BarrierAccessFlags::kShaderWrite
                | Core::BarrierAccessFlags::kCopySource;
            graph.GetResourcePool()->CommitBufferMemory(buffer.Get(), commitParams);
            return buffer;
        }


        struct CacheChanges final
        {
            //! Every page, including the atlas page owners, is discarded.
            bool m_isCreated = false;
            //! The pages of the clip levels are discarded.
            bool m_isSunReset = false;
        };


        // Recreates the cache when the level count changes.
        CacheChanges PrepareCache(Core::FrameGraph& graph, const PageInputs& inputs, PageCache& cache)
        {
            const uint32_t levelCount = inputs.m_settings.m_levelCount;
            CacheChanges changes;
            changes.m_isCreated = cache.m_pageTable == nullptr || cache.m_levelCount != levelCount;
            if (changes.m_isCreated)
            {
                const uint32_t entryCount = GetBlockCount(levelCount) * kSunShadowLevelPageCount;
                cache.m_levelCount = levelCount;
                cache.m_pageTable = CreatePersistentBuffer(graph, "SunShadowPageTable", entryCount);
                cache.m_owners = CreatePersistentBuffer(graph, "SunShadowPageOwners", kSunShadowPhysicalPageCount);

                const Vector2UInt atlasSize(kSunShadowAtlasSize, kSunShadowAtlasSize);
                cache.m_staticAtlas =
                    Core::Texture::Create(graph.GetDevice(), "SunShadowStaticAtlas", Core::Format::kR32_UINT, atlasSize);
                Core::ResourceCommitParams commitParams;
                commitParams.m_memory = Core::ResourceMemory::kDeviceLocal;
                commitParams.m_bindFlags = Core::BarrierAccessFlags::kShaderRead | Core::BarrierAccessFlags::kShaderWrite;
                graph.GetResourcePool()->CommitTextureMemory(cache.m_staticAtlas.Get(), commitParams);
            }

            // Cached depth is relative to the light orientation and the snapped near plane.
            const Settings& previous = cache.m_settings;
            const bool isLayoutChanged = previous.m_level0Size != inputs.m_settings.m_level0Size
                || previous.m_depthRange != inputs.m_settings.m_depthRange;
            const bool isLightChanged =
                cache.m_directionToLight != inputs.m_directionToLight || cache.m_hasSun != inputs.m_hasSun;
            const bool isDepthChanged = cache.m_depthOrigin != inputs.m_levels[0].m_depthOrigin;
            changes.m_isSunReset = changes.m_isCreated || isLayoutChanged || isLightChanged || isDepthChanged;
            return changes;
        }


        void BuildBlocks(const PageInputs& inputs, const uint32_t levelCount,
                         festd::fixed_vector<SunShadowBlock, kSunShadowMaxBlockCount>& blocks)
        {
            blocks.clear();
            for (uint32_t levelIndex = 0; levelIndex < levelCount; ++levelIndex)
            {
                SunShadowBlock& block = blocks.emplace_back();
                block.m_viewProjection = inputs.m_levels[levelIndex].m_viewProjection;
                block.m_pageOrigin = inputs.m_levels[levelIndex].m_pageOrigin;
                block.m_pagesPerAxis = inputs.m_hasSun ? kSunShadowLevelPagesPerAxis : 0;
                block.m_isPerspective = 0;
            }

            const uint32_t localBlockCount = kLocalShadowMaxMapCount * kLocalShadowMipCount;
            FE_Assert(inputs.m_localBlocks.empty() || inputs.m_localBlocks.size() == localBlockCount);
            for (uint32_t blockIndex = 0; blockIndex < localBlockCount; ++blockIndex)
                blocks.push_back(inputs.m_localBlocks.empty() ? SunShadowBlock{} : inputs.m_localBlocks[blockIndex]);
        }


        // Fences complete in submission order, so the last completed readback holds the newest counts.
        void ResolveFeedback(PageCache& cache)
        {
            FeedbackReadback* latest = nullptr;
            for (FeedbackReadback& readback : cache.m_readbacks)
            {
                if (!readback.m_isPending || !readback.m_fence.IsReady())
                    continue;

                readback.m_isPending = false;
                if (latest == nullptr || readback.m_fence.m_value > latest->m_fence.m_value)
                    latest = &readback;
            }

            if (latest == nullptr)
                return;

            const auto* counters = static_cast<const uint32_t*>(latest->m_buffer->Map());
            latest->m_buffer->InvalidateMappedRange(0, kSunShadowCounterCount * sizeof(uint32_t));
            for (uint32_t blockIndex = 0; blockIndex < kSunShadowMaxBlockCount; ++blockIndex)
            {
                cache.m_requestedPages[blockIndex] = counters[kSunShadowBlockRequestCounter + blockIndex];
                cache.m_dirtyPages[blockIndex] = counters[kSunShadowBlockDirtyCounter + blockIndex];
            }

            latest->m_buffer->Unmap();
            cache.m_hasFeedback = true;
        }


        // A block draws its casters until its counts are read back, then only while it has requested or dirty pages.
        void DecideBlockDraws(PageCache& cache, PageFrame& frame)
        {
            for (uint32_t blockIndex = 0; blockIndex < frame.m_blocks.size(); ++blockIndex)
            {
                const bool isUsed = frame.m_blocks[blockIndex].m_pagesPerAxis != 0;
                const bool isRecent = !cache.m_hasFeedback || cache.m_blockAges[blockIndex] < kBlockFeedbackLatency;
                frame.m_drawsStatic[blockIndex] = isUsed && (isRecent || cache.m_dirtyPages[blockIndex] != 0);
                frame.m_drawsDynamic[blockIndex] = isUsed && (isRecent || cache.m_requestedPages[blockIndex] != 0);
            }
        }


        void AddPageMaskPasses(Core::FrameGraph& graph, const PageCache& cache, const PageFrame& frame,
                               const uint32_t requiredFlags,
                               festd::fixed_vector<Tools::DepthPyramid::Output, kSunShadowMaxBlockCount>& pyramids)
        {
            Tools::DepthPyramid::Settings pyramidSettings;
            pyramidSettings.m_buildsClosest = false;
            pyramids.clear();
            const auto& draws = requiredFlags == kStaticPageFlags ? frame.m_drawsStatic : frame.m_drawsDynamic;
            for (uint32_t blockIndex = 0; blockIndex < frame.m_blocks.size(); ++blockIndex)
            {
                const SunShadowBlock& block = frame.m_blocks[blockIndex];
                if (!draws[blockIndex])
                {
                    pyramids.emplace_back();
                    continue;
                }

                const Vector2UInt maskSize(block.m_pagesPerAxis * kSunShadowMaskTexelsPerPage);
                Rc<Core::Texture> mask =
                    Core::Texture::Create(graph.GetDevice(), "SunShadowPageMask", Core::Format::kR32_SFLOAT, maskSize);
                auto* buildMask = graph.AllocatePassData<SunShadowPageMaskPassDesc>();
                buildMask->m_pipeline = BuildPageMaskPipeline::GetPipeline();
                buildMask->m_constants.m_pageTable = graph.GetSRV(cache.m_pageTable.Get());
                buildMask->m_constants.m_mask = graph.GetUAV(mask.Get());
                buildMask->m_constants.m_level = blockIndex;
                buildMask->m_constants.m_requiredFlags = requiredFlags;
                buildMask->m_constants.m_pageOrigin = block.m_pageOrigin;
                buildMask->m_constants.m_pagesPerAxis = block.m_pagesPerAxis;
                graph.AddDispatchPass("BuildPageMask", buildMask, Math::CeilDivide(maskSize, kSunShadowTileSize));
                pyramids.push_back(Tools::DepthPyramid::AddPass(graph, mask.Get(), pyramidSettings).m_farthest);
            }
        }


        void AddInvalidatePass(Core::FrameGraph& graph, Core::RingUploader& uploader, const PageInputs& inputs,
                               const PageCache& cache, const PageFrame& frame)
        {
            const uint32_t regionCount = inputs.m_staticChanges.size();
            Rc<Core::Buffer> regions =
                Core::Buffer::CreateStructured<PackedVector4F>(graph.GetDevice(), "SunShadowRegions", 2 * regionCount);
            const auto writeRegions = [&inputs](void* target) {
                auto* corners = static_cast<PackedVector4F*>(target);
                for (const Aabb& bounds : inputs.m_staticChanges)
                {
                    *corners++ = PackedVector4F(Vector4(bounds.min, 0.0f));
                    *corners++ = PackedVector4F(Vector4(bounds.max, 0.0f));
                }
            };
            FE_Verify(uploader.UploadGenerated(graph, regions.Get(), 2 * regionCount * sizeof(PackedVector4F), writeRegions));

            const uint32_t blockCount = frame.m_blocks.size();
            Rc<Core::Buffer> blocks =
                Core::Buffer::CreateStructured<SunShadowBlock>(graph.GetDevice(), "SunShadowBlocks", blockCount);
            FE_Verify(uploader.UploadArray(graph, blocks.Get(), festd::span<const SunShadowBlock>(frame.m_blocks)));

            auto* invalidate = graph.AllocatePassData<SunShadowInvalidatePassDesc>();
            invalidate->m_pipeline = InvalidatePagesPipeline::GetPipeline();
            invalidate->m_constants.m_blocks = graph.GetSRV(blocks.Get());
            invalidate->m_constants.m_regions = graph.GetSRV(regions.Get());
            invalidate->m_constants.m_pageTable = graph.GetUAV(cache.m_pageTable.Get());
            invalidate->m_constants.m_owners = graph.GetUAV(cache.m_owners.Get());
            graph.AddDispatchPass("InvalidatePages", invalidate, Vector2UInt(regionCount, blockCount));
        }
    } // namespace


    void AddRequestPasses(Core::FrameGraph& graph, Core::RingUploader& uploader, const PageInputs& inputs, PageCache& cache,
                          PageFrame& frame)
    {
        FE_PROFILER_ZONE();
        FE_Assert(inputs.m_levels.size() == inputs.m_settings.m_levelCount);

        const CacheChanges changes = PrepareCache(graph, inputs, cache);
        const uint32_t levelCount = cache.m_levelCount;
        const uint32_t blockCount = GetBlockCount(levelCount);
        frame.m_pageCount = blockCount * kSunShadowLevelPageCount;
        BuildBlocks(inputs, levelCount, frame.m_blocks);
        ResolveFeedback(cache);

        // Levels scroll by the whole pages their origins moved; a reset makes the previous origins irrelevant. Local maps do
        // not scroll, and a light that moved or changed discards the pages of its maps.
        festd::fixed_vector<SunShadowScroll, kSunShadowMaxBlockCount> scroll;
        for (uint32_t levelIndex = 0; levelIndex < levelCount; ++levelIndex)
        {
            const Vector2Int origin = inputs.m_levels[levelIndex].m_pageOrigin;
            const Vector2Int previousOrigin = changes.m_isSunReset ? origin : cache.m_pageOrigins[levelIndex];
            scroll.push_back({ previousOrigin, origin, changes.m_isSunReset ? 1u : 0u, 0 });
        }

        for (uint32_t blockIndex = levelCount; blockIndex < blockCount; ++blockIndex)
        {
            const uint32_t mapIndex = (blockIndex - levelCount) / kLocalShadowMipCount;
            const bool isReset = changes.m_isCreated || (inputs.m_localResetMask >> mapIndex & 1) != 0;
            scroll.push_back({ Vector2Int::kZero, Vector2Int::kZero, isReset ? 1u : 0u, 0 });
        }

        for (uint32_t blockIndex = 0; blockIndex < blockCount; ++blockIndex)
        {
            const bool isReset = scroll[blockIndex].m_isReset != 0;
            scroll[blockIndex].m_keepsDirty = !isReset && cache.m_isStaticSkipped[blockIndex] ? 1u : 0u;
            uint8_t& age = cache.m_blockAges[blockIndex];
            age = isReset ? 0 : static_cast<uint8_t>(Math::Min(age + 1u, 255u));
        }

        DecideBlockDraws(cache, frame);
        for (uint32_t blockIndex = 0; blockIndex < blockCount; ++blockIndex)
            cache.m_isStaticSkipped[blockIndex] = !frame.m_drawsStatic[blockIndex];

        cache.m_settings = inputs.m_settings;
        cache.m_directionToLight = inputs.m_directionToLight;
        cache.m_hasSun = inputs.m_hasSun;
        cache.m_depthOrigin = inputs.m_levels[0].m_depthOrigin;
        cache.m_pageOrigins.clear();
        for (const LevelSetup& level : inputs.m_levels)
            cache.m_pageOrigins.push_back(level.m_pageOrigin);

        Core::Device* device = graph.GetDevice();
        Rc<Core::Buffer> scrollBuffer = Core::Buffer::CreateStructured<SunShadowScroll>(device, "SunShadowScroll", blockCount);
        FE_Verify(uploader.UploadArray(graph, scrollBuffer.Get(), festd::span<const SunShadowScroll>(scroll)));
        frame.m_counters = Core::Buffer::CreateStructured<uint32_t>(device, "SunShadowCounters", kSunShadowCounterCount);
        frame.m_freePages = Core::Buffer::CreateStructured<uint32_t>(device, "SunShadowFreePages", kSunShadowPhysicalPageCount);
        frame.m_evictablePages =
            Core::Buffer::CreateStructured<uint32_t>(device, "SunShadowEvictablePages", kSunShadowPhysicalPageCount);
        frame.m_dirtyPages = Core::Buffer::CreateStructured<uint32_t>(device, "SunShadowDirtyPages", kSunShadowPhysicalPageCount);
        frame.m_atlas = Core::Texture::Create(device,
                                              "SunShadowAtlas",
                                              Core::Format::kR32_UINT,
                                              Vector2UInt(kSunShadowAtlasSize, kSunShadowAtlasSize));

        auto* update = graph.AllocatePassData<SunShadowUpdatePassDesc>();
        update->m_pipeline = UpdatePagesPipeline::GetPipeline();
        update->m_constants.m_pageTable = graph.GetUAV(cache.m_pageTable.Get());
        update->m_constants.m_owners = graph.GetUAV(cache.m_owners.Get());
        update->m_constants.m_counters = graph.GetUAV(frame.m_counters.Get());
        update->m_constants.m_scroll = graph.GetSRV(scrollBuffer.Get());
        update->m_constants.m_pageCount = frame.m_pageCount;
        update->m_constants.m_isReset = changes.m_isCreated ? 1 : 0;
        const uint32_t updateCount = Math::Max(frame.m_pageCount, kSunShadowPhysicalPageCount);
        graph.AddDispatchPass("UpdatePages", update, Math::CeilDivide(updateCount, kSunShadowThreadCount));

        if (!changes.m_isCreated && !inputs.m_staticChanges.empty())
            AddInvalidatePass(graph, uploader, inputs, cache, frame);

        const Vector2UInt resolution = inputs.m_depth->GetDesc().GetSize2D();
        auto* mark = graph.AllocatePassData<SunShadowMarkPassDesc>();
        mark->m_pipeline = MarkPagesPipeline::GetPipeline();
        mark->m_constants.m_data = graph.GetSRV(inputs.m_data);
        mark->m_constants.m_view = graph.GetSRV(inputs.m_view);
        mark->m_constants.m_depth = graph.GetSRV(inputs.m_depth);
        mark->m_constants.m_pageTable = graph.GetUAV(cache.m_pageTable.Get());
        mark->m_constants.m_localLighting = inputs.m_localLighting;
        if (inputs.m_localMaps != nullptr)
        {
            mark->m_constants.m_localMaps = graph.GetSRV(inputs.m_localMaps);
            mark->m_constants.m_localLightMaps = graph.GetSRV(inputs.m_localLightMaps);
        }

        mark->m_constants.m_resolution = resolution;
        graph.AddDispatchPass("MarkPages", mark, Math::CeilDivide(resolution, kSunShadowTileSize));
    }


    void AddAllocatePasses(Core::FrameGraph& graph, const PageCache& cache, PageFrame& frame)
    {
        FE_PROFILER_ZONE();

        auto* collect = graph.AllocatePassData<SunShadowCollectPassDesc>();
        collect->m_pipeline = CollectPagesPipeline::GetPipeline();
        collect->m_constants.m_pageTable = graph.GetSRV(cache.m_pageTable.Get());
        collect->m_constants.m_owners = graph.GetSRV(cache.m_owners.Get());
        collect->m_constants.m_freePages = graph.GetUAV(frame.m_freePages.Get());
        collect->m_constants.m_evictablePages = graph.GetUAV(frame.m_evictablePages.Get());
        collect->m_constants.m_counters = graph.GetUAV(frame.m_counters.Get());
        graph.AddDispatchPass("CollectPages", collect, Math::CeilDivide(kSunShadowPhysicalPageCount, kSunShadowThreadCount));

        auto* allocate = graph.AllocatePassData<SunShadowAllocatePassDesc>();
        allocate->m_pipeline = AllocatePagesPipeline::GetPipeline();
        allocate->m_constants.m_pageTable = graph.GetUAV(cache.m_pageTable.Get());
        allocate->m_constants.m_owners = graph.GetUAV(cache.m_owners.Get());
        allocate->m_constants.m_freePages = graph.GetSRV(frame.m_freePages.Get());
        allocate->m_constants.m_evictablePages = graph.GetSRV(frame.m_evictablePages.Get());
        allocate->m_constants.m_dirtyPages = graph.GetUAV(frame.m_dirtyPages.Get());
        allocate->m_constants.m_counters = graph.GetUAV(frame.m_counters.Get());
        allocate->m_constants.m_pageCount = frame.m_pageCount;
        graph.AddDispatchPass("AllocatePages", allocate, Math::CeilDivide(frame.m_pageCount, kSunShadowThreadCount));

        auto* clearAtlas = graph.AllocatePassData<SunShadowClearAtlasPassDesc>();
        clearAtlas->m_pipeline = ClearAtlasPagesPipeline::GetPipeline();
        clearAtlas->m_constants.m_counters = graph.GetSRV(frame.m_counters.Get());
        clearAtlas->m_constants.m_dirtyPages = graph.GetSRV(frame.m_dirtyPages.Get());
        clearAtlas->m_constants.m_atlas = graph.GetUAV(cache.m_staticAtlas.Get());
        graph.AddDispatchPass("ClearAtlasPages", clearAtlas, kSunShadowPhysicalPageCount);

        AddPageMaskPasses(graph, cache, frame, kStaticPageFlags, frame.m_staticPyramids);
        AddPageMaskPasses(graph, cache, frame, kDynamicPageFlags, frame.m_dynamicPyramids);
    }


    void ScheduleFeedbackReadback(Core::FrameGraph& graph, PageCache& cache, const PageFrame& frame,
                                  const Core::FenceSyncPoint fence)
    {
        // Skip this frame when every readback is still in flight.
        FeedbackReadback* readback = festd::find_if(cache.m_readbacks, [](const FeedbackReadback& candidate) {
            return !candidate.m_isPending;
        });
        if (readback == cache.m_readbacks.end())
            return;

        constexpr uint32_t kByteSize = kSunShadowCounterCount * sizeof(uint32_t);
        if (readback->m_buffer == nullptr)
        {
            readback->m_buffer = Core::Buffer::CreateByteAddress(graph.GetDevice(), "SunShadowFeedbackReadback", kByteSize);

            Core::ResourceCommitParams commitParams;
            commitParams.m_memory = Core::ResourceMemory::kHostRandomAccess;
            commitParams.m_bindFlags = Core::BarrierAccessFlags::kCopyDest;
            graph.GetResourcePool()->CommitBufferMemory(readback->m_buffer.Get(), commitParams);
        }

        graph.AddCopyPass(Core::BufferView(readback->m_buffer.Get()), Core::BufferView(frame.m_counters.Get()));
        readback->m_fence = fence;
        readback->m_isPending = true;
    }


    void AddCopyPagesPass(Core::FrameGraph& graph, const PageCache& cache, const PageFrame& frame)
    {
        auto* copy = graph.AllocatePassData<SunShadowCopyPagesPassDesc>();
        copy->m_pipeline = CopyPagesPipeline::GetPipeline();
        copy->m_constants.m_pageTable = graph.GetSRV(cache.m_pageTable.Get());
        copy->m_constants.m_owners = graph.GetSRV(cache.m_owners.Get());
        copy->m_constants.m_source = graph.GetSRV(cache.m_staticAtlas.Get());
        copy->m_constants.m_target = graph.GetUAV(frame.m_atlas.Get());
        graph.AddDispatchPass("CopyPages", copy, kSunShadowPhysicalPageCount);
    }


    bool Settings::Validate() const
    {
        const bool isLevelCountValid = m_levelCount > 0 && m_levelCount <= kSunShadowMaxLevelCount;
        return isLevelCountValid && m_level0Size > 0.0f && m_depthRange > 0.0f;
    }


    void CalculateLevels(const Settings& settings, const Vector3 cameraPosition, const Vector3 directionToLight,
                         festd::fixed_vector<LevelSetup, kSunShadowMaxLevelCount>& levels)
    {
        FE_Assert(settings.Validate());

        const Quaternion rotation = CalculateLightRotation(directionToLight);
        const Vector3 lightSpaceCamera = Math::Rotate(cameraPosition, Math::Invert(rotation));

        // The near plane moves in quarters of the depth range, which keeps the camera in the middle half of the range and
        // the cached depth valid until it moves to the next quarter.
        const float depthStep = settings.m_depthRange * 0.25f;
        const float depthOrigin = Math::Floor(lightSpaceCamera.z / depthStep) * depthStep - settings.m_depthRange * 0.5f;

        levels.clear();
        for (uint32_t levelIndex = 0; levelIndex < settings.m_levelCount; ++levelIndex)
        {
            // Moving the level in whole pages keeps every texel at the same world position while the camera moves.
            const float size = settings.m_level0Size * float(1u << levelIndex);
            const float pageSize = size / kSunShadowLevelPagesPerAxis;
            const Vector2Int pageOrigin(int32_t(Math::Floor(lightSpaceCamera.x / pageSize)),
                                        int32_t(Math::Floor(lightSpaceCamera.y / pageSize)));
            const Vector3 snapped(float(pageOrigin.x) * pageSize, float(pageOrigin.y) * pageSize, depthOrigin);

            LevelSetup& level = levels.emplace_back();
            level.m_camera = Transform::Create(Math::Rotate(snapped, rotation), rotation, 1.0f);
            level.m_size = size;
            level.m_texelSize = size / kSunShadowVirtualSize;
            level.m_pageOrigin = pageOrigin;
            level.m_depthOrigin = depthOrigin;

            // Matches View::SetCameraTransform followed by View::SetOrthographicProjection.
            const Matrix4x4 viewMatrix = Transform::ToMatrix(Math::Invert(level.m_camera));
            level.m_viewProjection = viewMatrix * Matrix4x4::Orthographic(size, size, 0.0f, settings.m_depthRange);
        }
    }


    SunShadowData BuildShadowData(const Settings& settings, const festd::span<const LevelSetup> levels, const CameraDesc& camera)
    {
        FE_Assert(levels.size() == settings.m_levelCount);

        SunShadowData data{};
        for (uint32_t levelIndex = 0; levelIndex < levels.size(); ++levelIndex)
        {
            data.m_levels[levelIndex].m_viewProjection = levels[levelIndex].m_viewProjection;
            data.m_levels[levelIndex].m_texelSize = levels[levelIndex].m_texelSize;
            data.m_levels[levelIndex].m_pageOrigin = levels[levelIndex].m_pageOrigin;
        }

        data.m_cameraPosition = PackedVector4F(Vector4(camera.m_position, camera.m_pixelFootprint));
        data.m_levelCount = settings.m_levelCount;
        data.m_isOrthographicCamera = camera.m_isOrthographic ? 1 : 0;
        data.m_level0TexelSize = settings.m_level0Size / kSunShadowVirtualSize;
        data.m_depthPerUnit = 1.0f / settings.m_depthRange;
        return data;
    }


    ViewModule::ViewModule(View* view)
        : ViewModuleBase(view)
        , m_pageCache(festd::make_unique<PageCache>())
    {
    }


    ViewModule::~ViewModule() = default;


    void ViewModule::DoRelease()
    {
        Memory::DefaultDelete(this);
    }


    View* ViewModule::GetLocalView(const uint32_t mapIndex, const uint32_t mip, const Layer layer)
    {
        Rc<View>& localView = m_localViews[festd::to_underlying(layer)][mapIndex * kLocalShadowMipCount + mip];
        if (localView == nullptr)
        {
            localView = m_view->GetScene()->CreateChildView();
            localView->GetModules().Add<ShadowAtlas::ViewModule>();
        }

        return localView.Get();
    }


    Culling::Stats ViewModule::GetCullingStats(const Layer layer) const
    {
        const auto& levelViews = m_levelViews[festd::to_underlying(layer)];

        Culling::Stats total;
        total.m_isValid = !levelViews.empty();
        for (const Rc<View>& levelView : levelViews)
        {
            const Culling::Stats& stats = levelView->GetModules().Find<ShadowAtlas::ViewModule>().GetStats();
            total.m_isValid &= stats.m_isValid;
            total.m_instanceCandidates += stats.m_instanceCandidates;
            total.m_instanceFrustumCulled += stats.m_instanceFrustumCulled;
            total.m_instanceOcclusionCulled += stats.m_instanceOcclusionCulled;
            total.m_instancesRendered += stats.m_instancesRendered;
            total.m_meshletCandidates += stats.m_meshletCandidates;
            total.m_meshletFrustumCulled += stats.m_meshletFrustumCulled;
            total.m_meshletOcclusionCulled += stats.m_meshletOcclusionCulled;
            total.m_meshletsRendered += stats.m_meshletsRendered;
            total.m_trianglesRendered += stats.m_trianglesRendered;
        }

        return total;
    }


    festd::span<const Env::Name> ViewModule::GetDebugModes() const
    {
        return GetModeNames();
    }


    void ViewModule::SetDebugMode(const uint32_t modeIndex)
    {
        FE_Assert(modeIndex == kInvalidIndex || modeIndex < festd::size(kDebugModes));
        m_debugMode = modeIndex;
    }


    void ViewModule::Update(Core::FrameGraphBlackboard& blackboard)
    {
        FE_Assert(m_settings.Validate());

        for (auto& levelViews : m_levelViews)
        {
            while (levelViews.size() < m_settings.m_levelCount)
            {
                Rc<View> levelView = m_view->GetScene()->CreateChildView();
                levelView->GetModules().Add<ShadowAtlas::ViewModule>();
                levelViews.push_back(std::move(levelView));
            }

            levelViews.resize(m_settings.m_levelCount);
        }

        blackboard.Add<ViewModule*>(this);

        if (m_debugMode == kInvalidIndex)
            return;

        blackboard.Add<DebugPassData>(kDebugModes[m_debugMode]);
        blackboard.Add<ViewColorOverride>(true);
    }


    namespace
    {
        void AddLevelPasses(Core::FrameGraph& graph, View* levelView, const uint32_t resolution,
                            const ShadowAtlas::LevelInputs& inputs)
        {
            ShadowAtlas::ViewModule& atlas = levelView->GetModules().Find<ShadowAtlas::ViewModule>();
            atlas.m_inputs = inputs;
            Renderer::Get().AddViewPasses(graph, { levelView, Vector2UInt(resolution, resolution) });
            atlas.m_inputs = {};
        }


        //! Shader data of the local maps of a camera view, in the blocks that follow the clip levels.
        struct LocalShadowFrame final
        {
            festd::array<LocalShadowMapData, kLocalShadowMaxMapCount> m_maps{};
            festd::fixed_vector<SunShadowBlock, kLocalShadowMaxMapCount * kLocalShadowMipCount> m_blocks;
            Rc<Core::Buffer> m_mapBuffer;
            Rc<Core::Buffer> m_lightMapBuffer;
            uint32_t m_resetMask = 0;
            bool m_hasMaps = false;
        };


        uint32_t ReleaseLocalMaps(LocalShadows::MapAllocation& allocation)
        {
            uint32_t resetMask = 0;
            for (uint32_t mapIndex = 0; mapIndex < LocalShadows::kMaxMapCount; ++mapIndex)
            {
                if (!allocation.m_maps[mapIndex].m_isUsed)
                    continue;

                allocation.m_maps[mapIndex] = {};
                resetMask |= 1u << mapIndex;
            }

            return resetMask;
        }


        void PrepareLocalShadows(Core::FrameGraph& graph, const RendererViewData& view, ViewModule& module,
                                 const LocalLightingSceneModule* lights, const bool isLit, LocalShadowFrame& frame)
        {
            LocalShadows::MapAllocation& allocation = module.GetLocalMaps();
            if (lights == nullptr || !isLit)
            {
                frame.m_resetMask = ReleaseLocalMaps(allocation);
                return;
            }

            LocalShadows::CameraDesc camera;
            camera.m_frustum = Frustum(view.m_view->GetViewProjectionMatrix());
            camera.m_position = view.m_view->GetCameraTransform().Translation();
            frame.m_resetMask = LocalShadows::SelectMaps(*lights, camera, allocation);

            const uint32_t firstBlock = module.m_settings.m_levelCount;
            for (uint32_t mapIndex = 0; mapIndex < LocalShadows::kMaxMapCount; ++mapIndex)
            {
                const LocalShadows::MapSetup& map = allocation.m_maps[mapIndex];
                const uint32_t mapBlock = firstBlock + mapIndex * kLocalShadowMipCount;
                frame.m_hasMaps |= map.m_isUsed;
                if (map.m_isUsed)
                    frame.m_maps[mapIndex] = LocalShadows::BuildMapData(map, mapBlock);

                for (uint32_t mip = 0; mip < kLocalShadowMipCount; ++mip)
                {
                    SunShadowBlock& block = frame.m_blocks.emplace_back();
                    block.m_viewProjection = map.m_viewProjection;
                    block.m_pageOrigin = Vector2Int::kZero;
                    block.m_pagesPerAxis = map.m_isUsed ? kSunShadowLevelPagesPerAxis >> mip : 0;
                    block.m_isPerspective = 1;
                }
            }

            if (!frame.m_hasMaps)
                return;

            Core::RingUploader& uploader = *view.m_renderQueueUploader;
            frame.m_mapBuffer = Core::Buffer::CreateStructured<LocalShadowMapData>(graph.GetDevice(),
                                                                                   "LocalShadowMaps",
                                                                                   LocalShadows::kMaxMapCount);
            FE_Verify(uploader.UploadArray(graph, frame.m_mapBuffer.Get(), festd::span<const LocalShadowMapData>(frame.m_maps)));

            // The first map of every shadowed light, by light slot.
            const uint32_t slotCount = Math::Max(lights->GetLightSlotCount(), 1u);
            frame.m_lightMapBuffer =
                Core::Buffer::CreateStructured<uint32_t>(graph.GetDevice(), "LocalShadowLightMaps", slotCount);
            const auto writeLightMaps = [&allocation, slotCount](void* target) {
                auto* lightMaps = static_cast<uint32_t*>(target);
                festd::fill_n(lightMaps, slotCount, kLocalShadowNoMap);
                for (uint32_t mapIndex = 0; mapIndex < LocalShadows::kMaxMapCount; ++mapIndex)
                {
                    const LocalShadows::MapSetup& map = allocation.m_maps[mapIndex];
                    if (!map.IsFirstFace() || map.m_light.m_value >= slotCount)
                        continue;

                    const bool isPoint = map.m_faceCount == kLocalShadowPointFaceCount;
                    lightMaps[map.m_light.m_value] = mapIndex | (isPoint ? kLocalShadowPointFlag : 0);
                }
            };
            FE_Verify(
                uploader.UploadGenerated(graph, frame.m_lightMapBuffer.Get(), slotCount * sizeof(uint32_t), writeLightMaps));

            // The child views of the mips project like the maps; the mips differ only in resolution.
            for (uint32_t mapIndex = 0; mapIndex < LocalShadows::kMaxMapCount; ++mapIndex)
            {
                const LocalShadows::MapSetup& map = allocation.m_maps[mapIndex];
                if (!map.m_isUsed)
                    continue;

                for (uint32_t mip = 0; mip < kLocalShadowMipCount; ++mip)
                {
                    for (uint32_t layerIndex = 0; layerIndex < kLayerCount; ++layerIndex)
                    {
                        View& mipView = *module.GetLocalView(mapIndex, mip, static_cast<Layer>(layerIndex));
                        mipView.SetCameraTransform(map.m_camera);
                        mipView.SetProjection(map.m_fovY, 1.0f, map.m_near, map.m_far);
                    }
                }
            }
        }


        //! Render the casters of one layer into the clip levels and the mips of the local maps.
        void AddLayerPasses(Core::FrameGraph& graph, ViewModule& module, const Layer layer, const bool hasSun,
                            festd::span<const LevelSetup> levels, const PageCache& cache, const PageFrame& frame,
                            Core::Texture* atlas)
        {
            const bool isStatic = layer == Layer::kStatic;
            const auto& pyramids = isStatic ? frame.m_staticPyramids : frame.m_dynamicPyramids;
            const DrawTag drawTag = isStatic ? DrawTags::Static : DrawTags::Dynamic;
            const uint32_t requiredFlags = isStatic ? kStaticPageFlags : kDynamicPageFlags;
            const uint32_t levelCount = module.m_settings.m_levelCount;
            const auto& draws = isStatic ? frame.m_drawsStatic : frame.m_drawsDynamic;
            for (uint32_t levelIndex = 0; hasSun && levelIndex < levelCount; ++levelIndex)
            {
                if (!draws[levelIndex])
                    continue;

                FE_FG_SCOPE(graph, Fmt::FixedFormat(isStatic ? "StaticLevel{}" : "DynamicLevel{}", levelIndex));
                AddLevelPasses(graph,
                               module.GetLevelView(levelIndex, layer),
                               kSunShadowVirtualSize,
                               { cache.m_pageTable.Get(),
                                 atlas,
                                 &pyramids[levelIndex],
                                 drawTag,
                                 requiredFlags,
                                 levels[levelIndex].m_pageOrigin,
                                 levelIndex });
            }

            const LocalShadows::MapAllocation& allocation = module.GetLocalMaps();
            for (uint32_t mapIndex = 0; mapIndex < LocalShadows::kMaxMapCount; ++mapIndex)
            {
                if (!allocation.m_maps[mapIndex].m_isUsed)
                    continue;

                for (uint32_t mip = 0; mip < kLocalShadowMipCount; ++mip)
                {
                    const uint32_t block = levelCount + mapIndex * kLocalShadowMipCount + mip;
                    if (!draws[block])
                        continue;

                    FE_FG_SCOPE(graph, Fmt::FixedFormat(isStatic ? "StaticMap{}Mip{}" : "DynamicMap{}Mip{}", mapIndex, mip));
                    AddLevelPasses(
                        graph,
                        module.GetLocalView(mapIndex, mip, layer),
                        kSunShadowVirtualSize >> mip,
                        { cache.m_pageTable.Get(), atlas, &pyramids[block], drawTag, requiredFlags, Vector2Int::kZero, block });
                }
            }
        }


        // Transparent surfaces are absent from the camera depth, so they rasterize their own requests in front of it.
        void AddTransparentRequestPasses(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard,
                                         const PageInputs& inputs, Core::Buffer* pageTable)
        {
            if (!blackboard.Contains<ForwardTransparency::ViewModule*>())
                return;

            const RendererViewData& view = blackboard.Get<RendererViewData>();
            SunShadowMarkRasterData markData{};
            markData.m_data = graph.GetSRV(inputs.m_data);
            markData.m_pageTable = graph.GetUAV(pageTable);
            markData.m_localLighting = inputs.m_localLighting;
            Rc<Core::Buffer> markBuffer =
                Core::Buffer::CreateStructured<SunShadowMarkRasterData>(graph.GetDevice(), "SunShadowMarkRaster", 1);
            graph.AddBufferDependency(markBuffer.Get(), inputs.m_data);
            if (inputs.m_localMaps != nullptr && inputs.m_localLighting.m_clusters.IsValid())
            {
                markData.m_localMaps = graph.GetSRV(inputs.m_localMaps);
                markData.m_localLightMaps = graph.GetSRV(inputs.m_localLightMaps);
                graph.AddBufferDependency(markBuffer.Get(), inputs.m_localMaps);
                graph.AddBufferDependency(markBuffer.Get(), inputs.m_localLightMaps);
                graph.AddBufferDependency(markBuffer.Get(), inputs.m_localLighting.m_clusters);
                graph.AddBufferDependency(markBuffer.Get(), inputs.m_localLighting.m_masks);
            }

            FE_Verify(view.m_renderQueueUploader->Upload(graph, markBuffer.Get(), markData));

            auto* base = graph.AllocatePassData<SunShadowMarkRasterPassDesc>();
            base->m_depthTarget = Core::TextureView::Create(view.m_mainDepthTarget);
            base->m_viewport = view.m_viewportRect;
            base->m_pageTable =
                Core::PassBufferAccess(Core::BufferView(pageTable),
                                       Core::BarrierSyncFlags::kPixelShading,
                                       Core::BarrierAccessFlags::kShaderRead | Core::BarrierAccessFlags::kShaderWrite);

            SceneRenderPass pass;
            pass.m_viewProjection = view.m_view->GetViewProjectionMatrix();
            pass.m_passDescToken = graph.AddBasePassDesc(base);
            pass.m_drawTag = DrawTags::Transparent;
            // Only materials with a ShadowPageMark technique receive shadows in the forward pass.
            pass.m_techniqueRole = Env::Name("ShadowPageMark");
            pass.m_independentCulling = true;
            pass.m_passData = markBuffer.Get();
            if (const auto* pyramids = blackboard.TryGet<DepthPyramidPass::Output>())
                pass.m_depthPyramid = &pyramids->m_farthest;

            view.m_scene->GetModules().ForEachActive([&](SceneModuleBase& module) {
                module.AddRenderPasses(graph, blackboard, *view.m_renderQueueUploader, pass);
            });
        }
    } // namespace


    void AddPasses(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard)
    {
        ViewModule* const* modulePtr = blackboard.TryGet<ViewModule*>();
        if (modulePtr == nullptr || blackboard.TryGet<Visibility::Output>() == nullptr)
            return;

        ViewModule& module = **modulePtr;
        const Settings& settings = module.m_settings;
        const RendererViewData& view = blackboard.Get<RendererViewData>();
        const auto* lighting = view.m_scene->GetModules().TryFind<GlobalLightingSceneModule>();
        const bool hasSun = lighting != nullptr && lighting->GetParameters().m_directionalLightIlluminanceLux > 0.0f;

        // Local lights are shadowed only in views that shade them, which is where their light clusters are built.
        const LocalLightingBinding* localLighting = blackboard.TryGet<LocalLightingBinding>();
        LocalShadowFrame localFrame;
        PrepareLocalShadows(graph,
                            view,
                            module,
                            view.m_scene->GetModules().TryFind<LocalLightingSceneModule>(),
                            localLighting != nullptr,
                            localFrame);
        if (!hasSun && !localFrame.m_hasMaps)
            return;

        FE_FG_SCOPE(graph, "SunShadow");

        // A camera pixel at unit distance covers 2 / (projection scale * height) world units.
        CameraDesc camera;
        camera.m_position = view.m_view->GetCameraTransform().Translation();
        camera.m_pixelFootprint = 2.0f / (view.m_view->GetProjectionMatrix().m_11 * view.m_viewportRect.Height());
        camera.m_isOrthographic = view.m_view->IsOrthographic();

        const Vector3 directionToLight = lighting != nullptr ? GetDirectionToLight(lighting->GetParameters()) : Vector3::AxisY();
        festd::fixed_vector<LevelSetup, kSunShadowMaxLevelCount> levels;
        CalculateLevels(settings, camera.m_position, directionToLight, levels);
        for (uint32_t levelIndex = 0; levelIndex < levels.size(); ++levelIndex)
        {
            const LevelSetup& level = levels[levelIndex];
            for (uint32_t layerIndex = 0; layerIndex < kLayerCount; ++layerIndex)
            {
                View& levelView = *module.GetLevelView(levelIndex, static_cast<Layer>(layerIndex));
                levelView.SetCameraTransform(level.m_camera);
                levelView.SetOrthographicProjection(level.m_size, level.m_size, 0.0f, settings.m_depthRange);
            }
        }

        // Without the sun no position projects into a clip level, while local maps still select mips by the camera.
        SunShadowData data = BuildShadowData(settings, levels, camera);
        if (!hasSun)
            data.m_levelCount = 0;

        Rc<Core::Buffer> dataBuffer = Core::Buffer::CreateStructured<SunShadowData>(graph.GetDevice(), "SunShadowData", 1);
        FE_Verify(view.m_renderQueueUploader->Upload(graph, dataBuffer.Get(), data));

        PageInputs inputs;
        inputs.m_data = dataBuffer.Get();
        inputs.m_view = view.m_constants.Get();
        inputs.m_depth = view.m_mainDepthTarget;
        inputs.m_settings = settings;
        inputs.m_directionToLight = directionToLight;
        inputs.m_levels = levels;
        inputs.m_hasSun = hasSun;
        inputs.m_localBlocks = localFrame.m_blocks;
        inputs.m_localResetMask = localFrame.m_resetMask;
        inputs.m_localMaps = localFrame.m_mapBuffer.Get();
        inputs.m_localLightMaps = localFrame.m_lightMapBuffer.Get();
        if (localLighting != nullptr && localFrame.m_hasMaps)
            inputs.m_localLighting = *localLighting;
        if (const auto* meshes = view.m_scene->GetModules().TryFind<MeshSceneModule>())
            inputs.m_staticChanges = meshes->GetStaticChanges();

        PageCache& cache = module.GetPageCache();
        PageFrame frame;
        AddRequestPasses(graph, *view.m_renderQueueUploader, inputs, cache, frame);
        AddTransparentRequestPasses(graph, blackboard, inputs, cache.m_pageTable.Get());
        AddAllocatePasses(graph, cache, frame);

        // The levels and mips render as child views between the page allocation and the passes that sample the atlas: static
        // casters into the dirty pages of the static atlas, then dynamic casters into this frame's copy of it.
        AddLayerPasses(graph, module, Layer::kStatic, hasSun, levels, cache, frame, cache.m_staticAtlas.Get());
        AddCopyPagesPass(graph, cache, frame);
        AddLayerPasses(graph, module, Layer::kDynamic, hasSun, levels, cache, frame, frame.m_atlas.Get());
        ScheduleFeedbackReadback(graph, cache, frame, Renderer::Get().GetGraphicsQueue()->GetCurrentFence());

        SunShadowBinding& binding = blackboard.Add<SunShadowBinding>();
        binding.m_data = graph.GetSRV(dataBuffer.Get());
        binding.m_pageTable = graph.GetSRV(cache.m_pageTable.Get());
        binding.m_atlas = graph.GetSRV(frame.m_atlas.Get());
        if (localFrame.m_hasMaps)
        {
            binding.m_localMaps = graph.GetSRV(localFrame.m_mapBuffer.Get());
            binding.m_localLightMaps = graph.GetSRV(localFrame.m_lightMapBuffer.Get());
        }

        Output& output = blackboard.Add<Output>();
        output.m_data = std::move(dataBuffer);
        output.m_pageTable = cache.m_pageTable;
        output.m_atlas = std::move(frame.m_atlas);
        output.m_localMaps = std::move(localFrame.m_mapBuffer);
        output.m_localLightMaps = std::move(localFrame.m_lightMapBuffer);
    }


    void AddDebugPasses(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard)
    {
        const DebugPassData* debugData = blackboard.TryGet<DebugPassData>();
        const SurfaceEvaluation::Output* surfaces = blackboard.TryGet<SurfaceEvaluation::Output>();
        if (debugData == nullptr || surfaces == nullptr)
            return;

        FE_FG_SCOPE(graph, "SunShadowDebug");

        RendererViewData& view = blackboard.Get<RendererViewData>();
        const Vector2UInt resolution = view.m_mainColorTarget->GetDesc().GetSize2D();
        auto* debug = graph.AllocatePassData<SunShadowDebugPassDesc>();
        debug->m_pipeline = SunShadowDebugPipeline::GetPipeline();
        if (const SunShadowBinding* binding = blackboard.TryGet<SunShadowBinding>())
            debug->m_constants.m_shadow = *binding;
        if (const LocalLightingBinding* localLighting = blackboard.TryGet<LocalLightingBinding>())
            debug->m_constants.m_localLighting = *localLighting;

        debug->m_constants.m_view = graph.GetSRV(view.m_constants.Get());
        debug->m_constants.m_depth = graph.GetSRV(view.m_mainDepthTarget);
        debug->m_constants.m_g1 = graph.GetSRV(surfaces->m_gBufferTargets[1].Get());
        debug->m_constants.m_output = graph.GetUAV(view.m_mainColorTarget);
        debug->m_constants.m_resolution = resolution;
        debug->m_constants.m_mode = debugData->m_mode;
        graph.AddDispatchPass("SunShadowDebug", debug, Math::CeilDivide(resolution, kSunShadowTileSize));
        view.m_isMainColorWritten = true;
    }
} // namespace FE::Graphics::SunShadow


namespace FE::Graphics::ShadowAtlas
{
    ViewModule::ViewModule(View* view)
        : ViewModuleBase(view)
    {
    }


    ViewModule::~ViewModule() = default;


    void ViewModule::DoRelease()
    {
        Memory::DefaultDelete(this);
    }


    void ViewModule::Update(Core::FrameGraphBlackboard& blackboard)
    {
        // Shadow views render once per frame; the page pyramid occludes casters that touch no drawn page.
        Culling::PassData& culling = blackboard.Add<Culling::PassData>();
        culling.m_settings.m_instanceOcclusionCulling = m_inputs.m_pagePyramid != nullptr;
        culling.m_settings.m_meshletOcclusionCulling = m_inputs.m_pagePyramid != nullptr;
        culling.m_occluderPyramid = m_inputs.m_pagePyramid;
        culling.m_stats = &m_stats;
        culling.m_viewProjection = m_view->GetViewProjectionMatrix();
        culling.m_occlusionViewProjection = culling.m_viewProjection;
        culling.m_cameraPosition = m_view->GetCameraTransform().Translation();
        culling.m_projectionScaleY = m_view->GetProjectionMatrix().m_11;
        culling.m_isOrthographic = m_view->IsOrthographic();

        if (m_inputs.m_atlas != nullptr)
            blackboard.Add<ViewModule*>(this);
    }


    void AddPasses(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard)
    {
        ViewModule* const* modulePtr = blackboard.TryGet<ViewModule*>();
        if (modulePtr == nullptr)
            return;

        FE_FG_SCOPE(graph, "ShadowAtlas");

        const LevelInputs& inputs = (*modulePtr)->m_inputs;
        const RendererViewData& view = blackboard.Get<RendererViewData>();
        const Core::TextureView atlas = Core::TextureView::Create(inputs.m_atlas);

        SunShadowRasterData rasterData{};
        rasterData.m_pageTable = graph.GetSRV(inputs.m_pageTable);
        rasterData.m_atlas = graph.GetUAV(atlas);
        rasterData.m_level = inputs.m_level;
        rasterData.m_requiredFlags = inputs.m_requiredPageFlags;
        rasterData.m_pageOrigin = inputs.m_pageOrigin;
        Rc<Core::Buffer> rasterBuffer =
            Core::Buffer::CreateStructured<SunShadowRasterData>(graph.GetDevice(), "SunShadowRaster", 1);
        graph.AddBufferDependency(rasterBuffer.Get(), inputs.m_pageTable);
        FE_Verify(view.m_renderQueueUploader->Upload(graph, rasterBuffer.Get(), rasterData));

        auto* base = graph.AllocatePassData<SunShadowRasterPassDesc>();
        base->m_viewport = view.m_viewportRect;
        base->m_atlas = Core::PassTextureAccess(atlas,
                                                Core::BarrierSyncFlags::kPixelShading,
                                                Core::BarrierAccessFlags::kShaderRead | Core::BarrierAccessFlags::kShaderWrite,
                                                Core::BarrierLayout::kShaderReadWrite);

        SceneRenderPass pass;
        pass.m_viewProjection = view.m_view->GetViewProjectionMatrix();
        pass.m_passDescToken = graph.AddBasePassDesc(base);
        pass.m_drawTag = inputs.m_drawTag;
        // Only materials with a ShadowDepth technique cast sun shadows.
        pass.m_techniqueRole = Env::Name("ShadowDepth");
        pass.m_phase = SceneRenderPhase::kSingle;
        pass.m_depthPyramid = Culling::GetPassData(blackboard).GetOccluderPyramid();
        pass.m_reportsStats = true;
        pass.m_passData = rasterBuffer.Get();
        view.m_scene->GetModules().ForEachActive([&](SceneModuleBase& module) {
            module.AddRenderPasses(graph, blackboard, *view.m_renderQueueUploader, pass);
        });
    }
} // namespace FE::Graphics::ShadowAtlas
