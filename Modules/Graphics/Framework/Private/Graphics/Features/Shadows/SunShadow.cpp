#include <Core/Math/Frustum.h>
#include <Graphics/Core/PipelineVariantSet.h>
#include <Graphics/Core/ResourcePool.h>
#include <Graphics/Features/Lighting/GlobalLightingSceneModule.h>
#include <Graphics/Features/Mesh/MeshSceneModule.h>
#include <Graphics/Features/Shadows/SunShadow.h>
#include <Graphics/Features/Shadows/SunShadowPages.h>
#include <Graphics/Passes/DrawTags.h>
#include <Graphics/Passes/ForwardTransparencyPass.h>
#include <Graphics/Passes/RendererPassCommon.h>
#include <Graphics/Passes/SurfaceEvaluationPass.h>
#include <Graphics/Passes/VisibilityPass.h>
#include <Graphics/Renderer.h>
#include <Graphics/Scene/Culling.h>
#include <Graphics/Scene/Scene.h>
#include <Graphics/Tables/MeshInstanceTable.h>

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
        constexpr uint32_t kDynamicPageFlags = kSunShadowPageAllocated | kSunShadowPageRequested | kSunShadowPageDynamic;


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


        Core::StructuredBuffer<uint32_t> CreatePersistentBuffer(Core::FrameGraph& graph, const Env::Name name,
                                                                const uint32_t count)
        {
            auto buffer = Core::Buffer::CreateStructured<uint32_t>(graph.GetDevice(), name, count);
            Core::ResourceCommitParams commitParams;
            commitParams.m_memory = Core::ResourceMemory::kDeviceLocal;
            // Copies read the cache back for diagnostics and tests.
            commitParams.m_bindFlags = Core::BarrierAccessFlags::kShaderRead | Core::BarrierAccessFlags::kShaderWrite
                | Core::BarrierAccessFlags::kCopySource | Core::BarrierAccessFlags::kCopyDest;
            graph.GetResourcePool()->CommitBufferMemory(buffer.m_buffer.Get(), commitParams);
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
                cache.m_feedback = CreatePersistentBuffer(graph, "SunShadowFeedback", Math::CeilDivide(entryCount, 32u));
                cache.m_levelBias = 0;
                cache.m_framesWithoutOverflow = 0;

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
                cache.m_dynamicPages[blockIndex] = counters[kSunShadowBlockDynamicCounter + blockIndex];
                cache.m_dirtyPages[blockIndex] = counters[kSunShadowBlockDirtyCounter + blockIndex];
            }

            cache.m_hasFeedback = true;
            if (counters[kSunShadowOverflowCounter] != 0)
            {
                cache.m_levelBias = Math::Min(cache.m_levelBias + 1, 3u);
                cache.m_framesWithoutOverflow = 0;
            }
            else if (++cache.m_framesWithoutOverflow >= 120)
            {
                if (cache.m_levelBias != 0)
                    --cache.m_levelBias;

                cache.m_framesWithoutOverflow = 0;
            }

            latest->m_buffer->Unmap();
        }


        // A block draws its casters until its counts are read back, then only while it has dynamic or dirty pages. The counts
        // are a few frames old; AllocatePages marks the pages that needed a skipped layer as undrawn.
        void DecideBlockDraws(const PageCache& cache, PageFrame& frame)
        {
            for (uint32_t blockIndex = 0; blockIndex < frame.m_blocks.size(); ++blockIndex)
            {
                const bool isUsed = frame.m_blocks[blockIndex].m_pagesPerAxis != 0;
                const bool isRecent = !cache.m_hasFeedback || cache.m_blockAges[blockIndex] < kBlockFeedbackLatency;
                frame.m_drawsStatic[blockIndex] = isUsed && (isRecent || cache.m_dirtyPages[blockIndex] != 0);
                frame.m_drawsDynamic[blockIndex] = isUsed && (isRecent || cache.m_dynamicPages[blockIndex] != 0);
            }
        }


        void AddPageMaskPasses(Core::FrameGraph& graph, const PageCache& cache, PageFrame& frame)
        {
            festd::fixed_vector<SunShadowPageMaskTarget, 2 * kSunShadowMaxBlockCount> targets;
            for (uint32_t layer = 0; layer < kLayerCount; ++layer)
            {
                auto& pyramids = layer == 0 ? frame.m_staticPyramids : frame.m_dynamicPyramids;
                const uint32_t flags = layer == 0 ? kStaticPageFlags : kDynamicPageFlags;
                pyramids.clear();
                for (uint32_t block = 0; block < frame.m_blocks.size(); ++block)
                {
                    Tools::DepthPyramid::Output& pyramid = pyramids.emplace_back();
                    const SunShadowBlock& setup = frame.m_blocks[block];
                    const bool isDrawn = layer == 0 ? frame.m_drawsStatic[block] : frame.m_drawsDynamic[block];
                    if (setup.m_pagesPerAxis == 0 || !isDrawn)
                        continue;

                    pyramid.m_size = Vector2UInt(setup.m_pagesPerAxis);
                    pyramid.m_mipCount = Math::FloorLog2(setup.m_pagesPerAxis) + 1;
                    pyramid.m_texture = Core::Texture::Create(graph.GetDevice(),
                                                              "SunShadowPagePyramid",
                                                              Core::Format::kR32_SFLOAT,
                                                              pyramid.m_size,
                                                              pyramid.m_mipCount);
                    SunShadowPageMaskTarget& target = targets.emplace_back();
                    target.m_level = block;
                    target.m_requiredFlags = flags;
                    target.m_pageOrigin = setup.m_pageOrigin;
                    target.m_pagesPerAxis = setup.m_pagesPerAxis;
                    for (uint32_t mip = 0; mip < pyramid.m_mipCount; ++mip)
                    {
                        const auto subresource = Core::TextureSubresource::Create(pyramid.m_texture->GetDesc(), mip, 0);
                        target.m_mips[mip] = graph.GetUAV(Core::TextureView::Create(pyramid.m_texture.Get(), subresource));
                    }
                }
            }

            if (targets.empty())
                return;

            SunShadowPageMaskConstants constants{};
            constants.m_pageTable = graph.GetSRV(cache.m_pageTable.Get());
            constants.m_targets = graph.CreateConstantsArray(festd::span<const SunShadowPageMaskTarget>(targets));
            graph.AddDispatch("BuildPageMasks", BuildPageMaskPipeline::GetPipeline(), constants, targets.size());
        }


        void AddInvalidatePass(Core::FrameGraph& graph, Core::RingUploader& uploader, const PageInputs& inputs,
                               const PageCache& cache, const PageFrame& frame, const bool marksDynamic = false)
        {
            const auto bounds = marksDynamic ? inputs.m_dynamicBounds : inputs.m_staticChanges;
            const uint32_t regionCount = bounds.size();
            auto regions = Core::Buffer::CreateStructured<PackedVector4F>(graph.GetDevice(), "SunShadowRegions", 2 * regionCount);
            const auto writeRegions = [bounds](void* target) {
                auto* corners = static_cast<PackedVector4F*>(target);
                for (const Aabb& region : bounds)
                {
                    *corners++ = PackedVector4F(Vector4(region.min, 0.0f));
                    *corners++ = PackedVector4F(Vector4(region.max, 0.0f));
                }
            };
            FE_Verify(uploader.UploadGenerated(graph, regions.Get(), 2 * regionCount * sizeof(PackedVector4F), writeRegions));

            const uint32_t blockCount = frame.m_blocks.size();
            auto blocks = Core::Buffer::CreateStructured<SunShadowBlock>(graph.GetDevice(), "SunShadowBlocks", blockCount);
            FE_Verify(uploader.UploadArray(graph, blocks.Get(), festd::span<const SunShadowBlock>(frame.m_blocks)));

            SunShadowInvalidateConstants invalidate{};
            invalidate.m_blocks = graph.GetSRV(blocks.Get());
            invalidate.m_regions = graph.GetSRV(regions.Get());
            invalidate.m_pageTable = graph.GetUAV(cache.m_pageTable.Get());
            invalidate.m_owners = graph.GetUAV(cache.m_owners.Get());
            invalidate.m_marksDynamic = marksDynamic ? 1u : 0u;
            graph.AddDispatch(marksDynamic ? "MarkDynamicPages" : "InvalidatePages",
                              InvalidatePagesPipeline::GetPipeline(),
                              invalidate,
                              Vector2UInt(regionCount, blockCount));
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
            scroll.push_back({ previousOrigin, origin, changes.m_isSunReset ? 1u : 0u, 0, 0, 0 });
        }

        for (uint32_t blockIndex = levelCount; blockIndex < blockCount; ++blockIndex)
        {
            const uint32_t mapIndex = (blockIndex - levelCount) / kLocalShadowMipCount;
            const bool isReset = changes.m_isCreated || (inputs.m_localResetMask >> mapIndex & 1) != 0;
            scroll.push_back({ Vector2Int::kZero, Vector2Int::kZero, isReset ? 1u : 0u, 0, 0, 0 });
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
        {
            cache.m_isStaticSkipped[blockIndex] = !frame.m_drawsStatic[blockIndex];
            scroll[blockIndex].m_skipsStatic = frame.m_drawsStatic[blockIndex] ? 0u : 1u;
            scroll[blockIndex].m_skipsDynamic = frame.m_drawsDynamic[blockIndex] ? 0u : 1u;
        }

        cache.m_settings = inputs.m_settings;
        cache.m_directionToLight = inputs.m_directionToLight;
        cache.m_hasSun = inputs.m_hasSun;
        cache.m_depthOrigin = inputs.m_levels[0].m_depthOrigin;
        cache.m_pageOrigins.clear();
        for (const LevelSetup& level : inputs.m_levels)
            cache.m_pageOrigins.push_back(level.m_pageOrigin);

        Core::Device* device = graph.GetDevice();
        frame.m_scroll = Core::Buffer::CreateStructured<SunShadowScroll>(device, "SunShadowScroll", blockCount);
        FE_Verify(uploader.UploadArray(graph, frame.m_scroll.Get(), festd::span<const SunShadowScroll>(scroll)));
        frame.m_counters = Core::Buffer::CreateStructured<uint32_t>(device, "SunShadowCounters", kSunShadowCounterCount);
        frame.m_freePages = Core::Buffer::CreateStructured<uint32_t>(device, "SunShadowFreePages", kSunShadowPhysicalPageCount);
        frame.m_evictablePages =
            Core::Buffer::CreateStructured<uint32_t>(device, "SunShadowEvictablePages", kSunShadowPhysicalPageCount);
        frame.m_dirtyPages = Core::Buffer::CreateStructured<uint32_t>(device, "SunShadowDirtyPages", kSunShadowPhysicalPageCount);
        frame.m_requestRanks = Core::Buffer::CreateStructured<uint32_t>(device, "SunShadowRequestRanks", frame.m_pageCount);
        frame.m_atlas = Core::Texture::Create(device,
                                              "SunShadowAtlas",
                                              Core::Format::kR32_UINT,
                                              Vector2UInt(kSunShadowAtlasSize, kSunShadowAtlasSize));

        SunShadowUpdateConstants update{};
        update.m_pageTable = graph.GetUAV(cache.m_pageTable.Get());
        update.m_owners = graph.GetUAV(cache.m_owners.Get());
        update.m_counters = graph.GetUAV(frame.m_counters.Get());
        update.m_scroll = graph.GetSRV(frame.m_scroll.Get());
        update.m_pageCount = frame.m_pageCount;
        update.m_isReset = changes.m_isCreated ? 1 : 0;
        update.m_feedback = graph.GetUAV(cache.m_feedback.Get());
        update.m_forceCoarsePages = inputs.m_settings.m_forceCoarsePages ? 1u : 0u;
        const uint32_t updateCount = Math::Max(frame.m_pageCount, kSunShadowPhysicalPageCount);
        graph.AddDispatch("UpdatePages",
                          UpdatePagesPipeline::GetPipeline(),
                          update,
                          Math::CeilDivide(updateCount, kSunShadowThreadCount));

        if (!changes.m_isCreated && !inputs.m_staticChanges.empty())
            AddInvalidatePass(graph, uploader, inputs, cache, frame);

        if (!inputs.m_dynamicBounds.empty())
            AddInvalidatePass(graph, uploader, inputs, cache, frame, true);

        const Vector2UInt resolution = inputs.m_depth->GetDesc().GetSize2D();
        SunShadowMarkConstants mark{};
        mark.m_data = inputs.m_data;
        mark.m_view = inputs.m_view;
        mark.m_depth = graph.GetSRV(inputs.m_depth);
        mark.m_pageTable = graph.GetUAV(cache.m_pageTable.Get());
        mark.m_localLighting = inputs.m_localLighting;
        if (inputs.m_localMaps != nullptr)
        {
            mark.m_localMaps = graph.GetSRV(inputs.m_localMaps);
            mark.m_localLightMaps = graph.GetSRV(inputs.m_localLightMaps);
        }

        mark.m_resolution = resolution;
        const bool forceCoarsePages = inputs.m_settings.m_forceCoarsePages;
        if (!forceCoarsePages)
        {
            graph.AddDispatch("MarkPages",
                              MarkPagesPipeline::GetPipeline(),
                              mark,
                              Math::CeilDivide(resolution, kSunShadowTileSize));
        }

        if (inputs.m_requestsCoarse || forceCoarsePages)
        {
            mark.m_isCoarse = 1;
            mark.m_coarseRect = PackedVector4F(forceCoarsePages ? Vector4(-1.0f, -1.0f, 1.0f, 1.0f) : inputs.m_coarseRect);
            mark.m_coarseRange = forceCoarsePages ? Vector2(0.0f, Constants::kMaxFloat) : inputs.m_coarseRange;
            graph.AddDispatch("MarkCoarsePages",
                              MarkPagesPipeline::GetPipeline(),
                              mark,
                              Math::CeilDivide(resolution, kSunShadowTileSize * 8));
        }
    }


    void AddAllocatePasses(Core::FrameGraph& graph, const PageCache& cache, PageFrame& frame)
    {
        FE_PROFILER_ZONE();

        SunShadowAllocateConstants allocate{};
        allocate.m_pageTable = graph.GetUAV(cache.m_pageTable.Get());
        allocate.m_owners = graph.GetUAV(cache.m_owners.Get());
        allocate.m_freePages = graph.GetSRV(frame.m_freePages.Get());
        allocate.m_evictablePages = graph.GetSRV(frame.m_evictablePages.Get());
        allocate.m_dirtyPages = graph.GetUAV(frame.m_dirtyPages.Get());
        allocate.m_counters = graph.GetUAV(frame.m_counters.Get());
        allocate.m_pageCount = frame.m_pageCount;
        allocate.m_levelCount = cache.m_levelCount;
        allocate.m_requestRanks = graph.GetUAV(frame.m_requestRanks.Get());
        allocate.m_scroll = graph.GetSRV(frame.m_scroll.Get());
        graph.AddDispatch("RankPageRequests", AllocatePagesPipeline::GetPipeline(), allocate, frame.m_blocks.size());
        allocate.m_phase = 1;
        graph.AddDispatch("PrioritizePages",
                          AllocatePagesPipeline::GetPipeline(),
                          allocate,
                          Math::CeilDivide(frame.m_pageCount, kSunShadowThreadCount));

        SunShadowCollectConstants collect{};
        collect.m_pageTable = graph.GetSRV(cache.m_pageTable.Get());
        collect.m_owners = graph.GetSRV(cache.m_owners.Get());
        collect.m_freePages = graph.GetUAV(frame.m_freePages.Get());
        collect.m_evictablePages = graph.GetUAV(frame.m_evictablePages.Get());
        collect.m_counters = graph.GetUAV(frame.m_counters.Get());
        graph.AddDispatch("CollectPages",
                          CollectPagesPipeline::GetPipeline(),
                          collect,
                          Math::CeilDivide(kSunShadowPhysicalPageCount, kSunShadowThreadCount));

        allocate.m_phase = 2;
        graph.AddDispatch("RankUnbackedPages", AllocatePagesPipeline::GetPipeline(), allocate, frame.m_blocks.size());
        allocate.m_phase = 3;
        graph.AddDispatch("AllocatePages",
                          AllocatePagesPipeline::GetPipeline(),
                          allocate,
                          Math::CeilDivide(frame.m_pageCount, kSunShadowThreadCount));

        SunShadowClearAtlasConstants clearAtlas{};
        clearAtlas.m_counters = graph.GetSRV(frame.m_counters.Get());
        clearAtlas.m_dirtyPages = graph.GetSRV(frame.m_dirtyPages.Get());
        clearAtlas.m_atlas = graph.GetUAV(cache.m_staticAtlas.Get());
        graph.AddDispatch("ClearAtlasPages", ClearAtlasPagesPipeline::GetPipeline(), clearAtlas, kSunShadowPhysicalPageCount);

        AddPageMaskPasses(graph, cache, frame);
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
        SunShadowCopyPagesConstants copy{};
        copy.m_pageTable = graph.GetSRV(cache.m_pageTable.Get());
        copy.m_owners = graph.GetSRV(cache.m_owners.Get());
        copy.m_source = graph.GetSRV(cache.m_staticAtlas.Get());
        copy.m_target = graph.GetUAV(frame.m_atlas.Get());
        copy.m_block = kSunShadowNoOwner;
        graph.AddDispatch("CopyPages", CopyPagesPipeline::GetPipeline(), copy, kSunShadowPhysicalPageCount);
    }


    bool Settings::Validate() const
    {
        const bool isLevelCountValid = m_levelCount > 0 && m_levelCount <= kSunShadowMaxLevelCount;
        return isLevelCountValid && m_level0Size > 0.0f && m_depthRange > 0.0f;
    }


    void CalculateLevels(const Settings& settings, const Vector3 cameraPosition, const Vector3 directionToLight,
                         festd::fixed_vector<LevelSetup, kSunShadowMaxLevelCount>& levels, const float* previousDepthOrigin)
    {
        FE_Assert(settings.Validate());

        const Quaternion rotation = CalculateLightRotation(directionToLight);
        const Vector3 lightSpaceCamera = Math::Rotate(cameraPosition, Math::Invert(rotation));

        // The near plane moves in quarters of the depth range, which keeps the camera in the middle half of the range and
        // the cached depth valid until it moves to the next quarter.
        const float depthStep = settings.m_depthRange * 0.25f;
        float depthOrigin = Math::Floor(lightSpaceCamera.z / depthStep + 0.5f) * depthStep - settings.m_depthRange * 0.5f;
        if (previousDepthOrigin != nullptr)
        {
            const float relativeDepth = (lightSpaceCamera.z - *previousDepthOrigin) / settings.m_depthRange;
            if (relativeDepth >= 0.25f && relativeDepth <= 0.75f)
                depthOrigin = *previousDepthOrigin;
        }

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


    festd::fixed_vector<SunShadowLevel, kSunShadowMaxLevelCount> BuildShadowLevels(const festd::span<const LevelSetup> levels)
    {
        festd::fixed_vector<SunShadowLevel, kSunShadowMaxLevelCount> result;
        for (const LevelSetup& level : levels)
        {
            SunShadowLevel& shaderLevel = result.emplace_back();
            shaderLevel.m_viewProjection = level.m_viewProjection;
            shaderLevel.m_texelSize = level.m_texelSize;
            shaderLevel.m_pageOrigin = level.m_pageOrigin;
        }

        return result;
    }


    SunShadowData BuildShadowData(const Settings& settings, const CameraDesc& camera)
    {
        SunShadowData data{};
        data.m_cameraPosition = PackedVector4F(Vector4(camera.m_position, camera.m_pixelFootprint));
        data.m_levelCount = settings.m_levelCount;
        data.m_isOrthographicCamera = camera.m_isOrthographic ? 1 : 0;
        data.m_level0TexelSize = settings.m_level0Size / kSunShadowVirtualSize;
        data.m_depthPerUnit = 1.0f / settings.m_depthRange;
        data.m_screenTraceTexels = settings.m_screenTraceTexels;
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

        blackboard.Add<ViewModule*>(this);

        if (m_debugMode == kInvalidIndex)
            return;

        blackboard.Add<DebugPassData>(kDebugModes[m_debugMode]);
        blackboard.Add<ViewColorOverride>(true);
    }


    namespace
    {
        //! Shader data of the local maps of a camera view, in the blocks that follow the clip levels.
        struct LocalShadowFrame final
        {
            festd::array<LocalShadowMapData, kLocalShadowMaxMapCount> m_maps{};
            festd::fixed_vector<SunShadowBlock, kLocalShadowMaxMapCount * kLocalShadowMipCount> m_blocks;
            Core::StructuredBuffer<LocalShadowMapData> m_mapBuffer;
            Core::StructuredBuffer<uint32_t> m_lightMapBuffer;
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
        }


        // Coarse pages are requested only where transparent surfaces can be: inside the screen rectangle of their bounds and
        // between their nearest and farthest distances from the camera.
        void SetCoarseRegion(const festd::span<const Aabb> transparentBounds, const View& view, const RectF& viewport,
                             PageInputs& inputs)
        {
            const Matrix4x4& viewProjection = view.GetViewProjectionMatrix();
            const Vector3 cameraPosition = view.GetCameraTransform().Translation();
            const Frustum frustum(viewProjection);

            Vector2 ndcMin(Constants::kMaxFloat);
            Vector2 ndcMax(-Constants::kMaxFloat);
            float nearDistance = Constants::kMaxFloat;
            float farDistance = 0.0f;
            bool isVisible = false;
            bool coversScreen = false;
            for (const Aabb& bounds : transparentBounds)
            {
                if (!frustum.Overlaps(bounds))
                    continue;

                isVisible = true;
                const Vector3 closest = Math::Max(bounds.min, Math::Min(bounds.max, cameraPosition));
                nearDistance = Math::Min(nearDistance, Math::Length(closest - cameraPosition));
                for (uint32_t corner = 0; corner < 8; ++corner)
                {
                    const Vector3 position((corner & 1) != 0 ? bounds.max.x : bounds.min.x,
                                           (corner & 2) != 0 ? bounds.max.y : bounds.min.y,
                                           (corner & 4) != 0 ? bounds.max.z : bounds.min.z);
                    farDistance = Math::Max(farDistance, Math::Length(position - cameraPosition));

                    // Bounds that reach behind the camera have no finite screen rectangle.
                    const Vector4 clip = Vector4(position, 1.0f) * viewProjection;
                    if (clip.w <= 1e-4f)
                    {
                        coversScreen = true;
                        continue;
                    }

                    const Vector2 ndc(clip.x / clip.w, clip.y / clip.w);
                    ndcMin = Math::Min(ndcMin, ndc);
                    ndcMax = Math::Max(ndcMax, ndc);
                }
            }

            inputs.m_requestsCoarse = isVisible;
            if (!isVisible)
                return;

            if (!coversScreen)
            {
                // Coarse requests are made per tile of eight pixels; keep the tiles that straddle the rectangle.
                const Vector2 margin(16.0f / viewport.Width(), 16.0f / viewport.Height());
                const Vector2 rectMin = Math::Max(ndcMin - margin, Vector2(-1.0f));
                const Vector2 rectMax = Math::Min(ndcMax + margin, Vector2(1.0f));
                inputs.m_coarseRect = Vector4(rectMin.x, rectMin.y, rectMax.x, rectMax.y);
            }

            // Distances along the rays of an orthographic camera are not distances from its position.
            if (!view.IsOrthographic())
                inputs.m_coarseRange = Vector2(nearDistance, farDistance);
        }


        // Culling inputs of one clip level or local map mip. LODs are selected for the resolution of the view, not for the
        // camera that samples it, and the page pyramid of the layer occludes the casters that touch no drawn page.
        Culling::PassData CreateViewCulling(const Matrix4x4& viewProjection, const Vector3 cameraPosition,
                                            const float projectionScaleY, const bool isOrthographic,
                                            const float lodErrorThreshold, const Tools::DepthPyramid::Output* pagePyramid)
        {
            Culling::PassData culling;
            culling.m_settings.m_instanceOcclusionCulling = true;
            culling.m_settings.m_meshletOcclusionCulling = true;
            culling.m_settings.m_lodErrorThreshold = lodErrorThreshold;
            culling.m_occluderPyramid = pagePyramid;
            culling.m_viewProjection = viewProjection;
            culling.m_occlusionViewProjection = viewProjection;
            culling.m_cameraPosition = cameraPosition;
            culling.m_projectionScaleY = projectionScaleY;
            culling.m_isOrthographic = isOrthographic;
            return culling;
        }


        // Static casters go into the dirty pages of the static atlas, then one copy moves the pages that dynamic casters touch
        // into this frame's atlas, then dynamic casters are added to it. The clip levels and local map mips of a layer that
        // have pages to draw are the views of one multi-view mesh pass, which culls and rasterizes them together.
        void AddBlockPasses(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard, ViewModule& module,
                            const bool hasSun, const festd::span<const LevelSetup> levels, const PageCache& cache,
                            const PageFrame& frame)
        {
            const RendererViewData& view = blackboard.Get<RendererViewData>();
            auto* meshes = view.m_scene->GetModules().TryFind<MeshSceneModule>();
            const Settings& settings = module.m_settings;
            const auto& maps = module.GetLocalMaps().m_maps;
            for (uint32_t layerIndex = 0; layerIndex < kLayerCount; ++layerIndex)
            {
                const Layer layer = static_cast<Layer>(layerIndex);
                const bool isStatic = layer == Layer::kStatic;
                if (!isStatic)
                    AddCopyPagesPass(graph, cache, frame);

                if (meshes == nullptr)
                    continue;

                const auto& draws = isStatic ? frame.m_drawsStatic : frame.m_drawsDynamic;
                const auto& pyramids = isStatic ? frame.m_staticPyramids : frame.m_dynamicPyramids;
                festd::inline_vector<Culling::PassData, 16> views;
                festd::inline_vector<Vector2, 16> viewSizes;
                festd::inline_vector<SunShadowRasterView, 16> rasterViews;
                const auto addView = [&](const uint32_t block, const uint32_t size, const Culling::PassData& culling) {
                    views.push_back(culling);
                    viewSizes.push_back(Vector2(float(size)));

                    SunShadowRasterView& rasterView = rasterViews.emplace_back();
                    rasterView.m_pageOrigin = frame.m_blocks[block].m_pageOrigin;
                    rasterView.m_level = block;
                    rasterView.m_size = size;
                };

                for (uint32_t level = 0; hasSun && level < levels.size(); ++level)
                {
                    if (!draws[level])
                        continue;

                    const LevelSetup& setup = levels[level];
                    addView(level,
                            kSunShadowVirtualSize,
                            CreateViewCulling(setup.m_viewProjection,
                                              setup.m_camera.Translation(),
                                              2.0f / setup.m_size,
                                              true,
                                              settings.m_lodErrorThreshold,
                                              &pyramids[level]));
                }

                for (uint32_t map = 0; map < maps.size(); ++map)
                {
                    if (!maps[map].m_isUsed)
                        continue;

                    // The mips of a map project alike and differ in resolution, so each selects its own LODs.
                    const float projectionScaleY = 1.0f / std::tan(maps[map].m_fovY * 0.5f);
                    for (uint32_t mip = 0; mip < kLocalShadowMipCount; ++mip)
                    {
                        const uint32_t block = settings.m_levelCount + map * kLocalShadowMipCount + mip;
                        if (!draws[block])
                            continue;

                        addView(block,
                                kSunShadowVirtualSize >> mip,
                                CreateViewCulling(maps[map].m_viewProjection,
                                                  maps[map].m_camera.Translation(),
                                                  projectionScaleY,
                                                  false,
                                                  settings.m_lodErrorThreshold,
                                                  &pyramids[block]));
                    }
                }

                // A layer without pages to draw has no views, and the pass then only keeps its counters current.
                MeshMultiViewPass pass;
                pass.m_stats = &module.GetLayerStats(layer);
                if (views.empty())
                {
                    meshes->AddMultiViewRenderPasses(graph, blackboard, *view.m_renderQueueUploader, pass);
                    continue;
                }

                FE_FG_SCOPE(graph, isStatic ? "StaticShadowViews" : "DynamicShadowViews");

                SunShadowRasterData rasterData{};
                rasterData.m_pageTable = graph.GetSRV(cache.m_pageTable.Get());
                rasterData.m_atlas = graph.GetUAV(isStatic ? cache.m_staticAtlas.Get() : frame.m_atlas.Get());
                rasterData.m_views = graph.CreateConstantsArray(festd::span<const SunShadowRasterView>(rasterViews));
                rasterData.m_requiredFlags = isStatic ? kStaticPageFlags : kDynamicPageFlags;

                // Every view draws into the top left corner of one viewport of the largest view size.
                auto* base = graph.AllocatePassData<SunShadowRasterPassDesc>();
                base->m_viewport = RectF::FromPosAndSize(Vector2::kZero, Vector2(float(kSunShadowVirtualSize)));

                pass.m_views = views;
                pass.m_viewSizes = viewSizes;
                pass.m_passDescToken = graph.AddBasePassDesc(base);
                pass.m_drawTag = isStatic ? DrawTags::Static : DrawTags::Dynamic;
                // Only materials with a ShadowDepth technique cast shadows.
                pass.m_techniqueRole = Env::Name("ShadowDepth");
                pass.m_passData = graph.CreateConstants(rasterData);
                meshes->AddMultiViewRenderPasses(graph, blackboard, *view.m_renderQueueUploader, pass);
            }
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
        PageCache& cache = module.GetPageCache();
        ResolveFeedback(cache);
        const float* depthOrigin = cache.m_pageTable != nullptr ? &cache.m_depthOrigin : nullptr;
        CalculateLevels(settings, camera.m_position, directionToLight, levels, depthOrigin);

        // Without the sun no position projects into a clip level, while local maps still select mips by the camera.
        const auto shaderLevels = BuildShadowLevels(levels);
        SunShadowData data = BuildShadowData(settings, camera);
        data.m_levelBias = float(cache.m_levelBias);
        data.m_levels = graph.CreateConstantsArray(festd::span<const SunShadowLevel>(shaderLevels));
        if (!hasSun)
            data.m_levelCount = 0;

        const ConstantsPtr<SunShadowData> dataConstants = graph.CreateConstants(data);

        PageInputs inputs;
        inputs.m_data = dataConstants;
        inputs.m_view = view.m_constants;
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
        {
            inputs.m_staticChanges = meshes->GetStaticChanges();
            inputs.m_dynamicBounds = meshes->GetDynamicBounds();

            // Only forward transparency samples shadows away from the camera depth.
            if (blackboard.Contains<ForwardTransparency::ViewModule*>())
                SetCoarseRegion(meshes->GetTransparentBounds(), *view.m_view, view.m_viewportRect, inputs);
        }

        PageFrame frame;
        AddRequestPasses(graph, *view.m_renderQueueUploader, inputs, cache, frame);
        AddAllocatePasses(graph, cache, frame);

        AddBlockPasses(graph, blackboard, module, hasSun, levels, cache, frame);
        ScheduleFeedbackReadback(graph, cache, frame, Renderer::Get().GetGraphicsQueue()->GetCurrentFence());

        SunShadowBinding& binding = blackboard.Add<SunShadowBinding>();
        binding.m_data = dataConstants;
        binding.m_pageTable = graph.GetSRV(cache.m_pageTable.Get());
        binding.m_atlas = graph.GetSRV(frame.m_atlas.Get());
        binding.m_staticAtlas = graph.GetSRV(cache.m_staticAtlas.Get());
        if (localFrame.m_hasMaps)
        {
            binding.m_localMaps = graph.GetSRV(localFrame.m_mapBuffer.Get());
            binding.m_localLightMaps = graph.GetSRV(localFrame.m_lightMapBuffer.Get());
        }

        Output& output = blackboard.Add<Output>();
        output.m_pageTable = cache.m_pageTable;
        output.m_staticAtlas = cache.m_staticAtlas;
        output.m_feedback = cache.m_feedback;
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
        SunShadowDebugConstants debug{};
        if (const SunShadowBinding* binding = blackboard.TryGet<SunShadowBinding>())
            debug.m_shadow = *binding;
        if (const LocalLightingBinding* localLighting = blackboard.TryGet<LocalLightingBinding>())
            debug.m_localLighting = *localLighting;

        debug.m_view = view.m_constants;
        debug.m_depth = graph.GetSRV(view.m_mainDepthTarget);
        debug.m_g1 = graph.GetSRV(surfaces->m_gBufferTargets[1].Get());
        debug.m_output = graph.GetUAV(view.m_mainColorTarget);
        debug.m_resolution = resolution;
        debug.m_mode = debugData->m_mode;
        graph.AddDispatch("SunShadowDebug",
                          SunShadowDebugPipeline::GetPipeline(),
                          debug,
                          Math::CeilDivide(resolution, kSunShadowTileSize));
        view.m_isMainColorWritten = true;
    }
} // namespace FE::Graphics::SunShadow
