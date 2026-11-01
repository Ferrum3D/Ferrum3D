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

        constexpr uint32_t kDebugModes[] = { kSunShadowDebugLevel, kSunShadowDebugPage, kSunShadowDebugFactor };

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
                                                    Env::Name("shadow-factor") };
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


        // Recreates the cache when the level count changes; returns whether every cached page must be discarded.
        bool PrepareCache(Core::FrameGraph& graph, const PageInputs& inputs, PageCache& cache)
        {
            const uint32_t levelCount = inputs.m_settings.m_levelCount;
            const bool isCreated = cache.m_pageTable == nullptr || cache.m_levelCount != levelCount;
            if (isCreated)
            {
                cache.m_levelCount = levelCount;
                cache.m_pageTable = CreatePersistentBuffer(graph, "SunShadowPageTable", levelCount * kSunShadowLevelPageCount);
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
            const bool isLightChanged = cache.m_directionToLight != inputs.m_directionToLight;
            const bool isDepthChanged = cache.m_depthOrigin != inputs.m_levels[0].m_depthOrigin;
            return isCreated || isLayoutChanged || isLightChanged || isDepthChanged;
        }


        void AddPageMaskPasses(Core::FrameGraph& graph, const PageInputs& inputs, const PageCache& cache,
                               const uint32_t requiredFlags,
                               festd::fixed_vector<Tools::DepthPyramid::Output, kSunShadowMaxLevelCount>& pyramids)
        {
            Tools::DepthPyramid::Settings pyramidSettings;
            pyramidSettings.m_buildsClosest = false;
            pyramids.clear();
            for (uint32_t levelIndex = 0; levelIndex < cache.m_levelCount; ++levelIndex)
            {
                const Vector2UInt maskSize(kSunShadowMaskSize, kSunShadowMaskSize);
                Rc<Core::Texture> mask =
                    Core::Texture::Create(graph.GetDevice(), "SunShadowPageMask", Core::Format::kR32_SFLOAT, maskSize);
                auto* buildMask = graph.AllocatePassData<SunShadowPageMaskPassDesc>();
                buildMask->m_pipeline = BuildPageMaskPipeline::GetPipeline();
                buildMask->m_constants.m_pageTable = graph.GetSRV(cache.m_pageTable.Get());
                buildMask->m_constants.m_mask = graph.GetUAV(mask.Get());
                buildMask->m_constants.m_level = levelIndex;
                buildMask->m_constants.m_requiredFlags = requiredFlags;
                buildMask->m_constants.m_pageOrigin = inputs.m_levels[levelIndex].m_pageOrigin;
                graph.AddDispatchPass("BuildPageMask", buildMask, Math::CeilDivide(maskSize, kSunShadowTileSize));
                pyramids.push_back(Tools::DepthPyramid::AddPass(graph, mask.Get(), pyramidSettings).m_farthest);
            }
        }


        void AddInvalidatePass(Core::FrameGraph& graph, Core::RingUploader& uploader, const PageInputs& inputs,
                               const PageCache& cache)
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

            auto* invalidate = graph.AllocatePassData<SunShadowInvalidatePassDesc>();
            invalidate->m_pipeline = InvalidatePagesPipeline::GetPipeline();
            invalidate->m_constants.m_data = graph.GetSRV(inputs.m_data);
            invalidate->m_constants.m_regions = graph.GetSRV(regions.Get());
            invalidate->m_constants.m_pageTable = graph.GetUAV(cache.m_pageTable.Get());
            invalidate->m_constants.m_owners = graph.GetUAV(cache.m_owners.Get());
            graph.AddDispatchPass("InvalidatePages", invalidate, Vector2UInt(regionCount, cache.m_levelCount));
        }
    } // namespace


    void AddRequestPasses(Core::FrameGraph& graph, Core::RingUploader& uploader, const PageInputs& inputs, PageCache& cache,
                          PageFrame& frame)
    {
        FE_PROFILER_ZONE();
        FE_Assert(inputs.m_levels.size() == inputs.m_settings.m_levelCount);

        const bool isReset = PrepareCache(graph, inputs, cache);
        const uint32_t levelCount = cache.m_levelCount;
        frame.m_pageCount = levelCount * kSunShadowLevelPageCount;

        // Levels scroll by the whole pages their origins moved; a reset makes the previous origins irrelevant.
        festd::fixed_vector<SunShadowScroll, kSunShadowMaxLevelCount> scroll;
        for (uint32_t levelIndex = 0; levelIndex < levelCount; ++levelIndex)
        {
            const Vector2Int origin = inputs.m_levels[levelIndex].m_pageOrigin;
            const Vector2Int previousOrigin = isReset ? origin : cache.m_pageOrigins[levelIndex];
            scroll.push_back({ previousOrigin, origin });
        }

        cache.m_settings = inputs.m_settings;
        cache.m_directionToLight = inputs.m_directionToLight;
        cache.m_depthOrigin = inputs.m_levels[0].m_depthOrigin;
        cache.m_pageOrigins.clear();
        for (const LevelSetup& level : inputs.m_levels)
            cache.m_pageOrigins.push_back(level.m_pageOrigin);

        Core::Device* device = graph.GetDevice();
        Rc<Core::Buffer> scrollBuffer = Core::Buffer::CreateStructured<SunShadowScroll>(device, "SunShadowScroll", levelCount);
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
        update->m_constants.m_isReset = isReset ? 1 : 0;
        const uint32_t updateCount = Math::Max(frame.m_pageCount, kSunShadowPhysicalPageCount);
        graph.AddDispatchPass("UpdatePages", update, Math::CeilDivide(updateCount, kSunShadowThreadCount));

        if (!isReset && !inputs.m_staticChanges.empty())
            AddInvalidatePass(graph, uploader, inputs, cache);

        const Vector2UInt resolution = inputs.m_depth->GetDesc().GetSize2D();
        auto* mark = graph.AllocatePassData<SunShadowMarkPassDesc>();
        mark->m_pipeline = MarkPagesPipeline::GetPipeline();
        mark->m_constants.m_data = graph.GetSRV(inputs.m_data);
        mark->m_constants.m_view = graph.GetSRV(inputs.m_view);
        mark->m_constants.m_depth = graph.GetSRV(inputs.m_depth);
        mark->m_constants.m_pageTable = graph.GetUAV(cache.m_pageTable.Get());
        mark->m_constants.m_resolution = resolution;
        graph.AddDispatchPass("MarkPages", mark, Math::CeilDivide(resolution, kSunShadowTileSize));
    }


    void AddAllocatePasses(Core::FrameGraph& graph, const PageInputs& inputs, const PageCache& cache, PageFrame& frame)
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

        AddPageMaskPasses(graph, inputs, cache, kStaticPageFlags, frame.m_staticPyramids);
        AddPageMaskPasses(graph, inputs, cache, kDynamicPageFlags, frame.m_dynamicPyramids);
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
        void AddLevelPasses(Core::FrameGraph& graph, ViewModule& module, const Layer layer,
                            const ShadowAtlas::LevelInputs& inputs)
        {
            View* levelView = module.GetLevelView(inputs.m_level, layer);
            ShadowAtlas::ViewModule& atlas = levelView->GetModules().Find<ShadowAtlas::ViewModule>();
            atlas.m_inputs = inputs;
            Renderer::Get().AddViewPasses(graph, { levelView, Vector2UInt(kSunShadowVirtualSize, kSunShadowVirtualSize) });
            atlas.m_inputs = {};
        }


        // Transparent surfaces are absent from the camera depth, so they rasterize their own requests in front of it.
        void AddTransparentRequestPasses(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard, Core::Buffer* data,
                                         Core::Buffer* pageTable)
        {
            if (!blackboard.Contains<ForwardTransparency::ViewModule*>())
                return;

            const RendererViewData& view = blackboard.Get<RendererViewData>();
            SunShadowMarkRasterData markData{};
            markData.m_data = graph.GetSRV(data);
            markData.m_pageTable = graph.GetUAV(pageTable);
            Rc<Core::Buffer> markBuffer =
                Core::Buffer::CreateStructured<SunShadowMarkRasterData>(graph.GetDevice(), "SunShadowMarkRaster", 1);
            graph.AddBufferDependency(markBuffer.Get(), data);
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
            // Only materials with a ShadowPageMark technique receive sun shadows in the forward pass.
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

        const RendererViewData& view = blackboard.Get<RendererViewData>();
        const auto* lighting = view.m_scene->GetModules().TryFind<GlobalLightingSceneModule>();
        if (lighting == nullptr || lighting->GetParameters().m_directionalLightIlluminanceLux <= 0.0f)
            return;

        FE_FG_SCOPE(graph, "SunShadow");

        ViewModule& module = **modulePtr;
        const Settings& settings = module.m_settings;

        // A camera pixel at unit distance covers 2 / (projection scale * height) world units.
        CameraDesc camera;
        camera.m_position = view.m_view->GetCameraTransform().Translation();
        camera.m_pixelFootprint = 2.0f / (view.m_view->GetProjectionMatrix().m_11 * view.m_viewportRect.Height());
        camera.m_isOrthographic = view.m_view->IsOrthographic();

        const Vector3 directionToLight = GetDirectionToLight(lighting->GetParameters());
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

        const SunShadowData data = BuildShadowData(settings, levels, camera);
        Rc<Core::Buffer> dataBuffer = Core::Buffer::CreateStructured<SunShadowData>(graph.GetDevice(), "SunShadowData", 1);
        FE_Verify(view.m_renderQueueUploader->Upload(graph, dataBuffer.Get(), data));

        PageInputs inputs;
        inputs.m_data = dataBuffer.Get();
        inputs.m_view = view.m_constants.Get();
        inputs.m_depth = view.m_mainDepthTarget;
        inputs.m_settings = settings;
        inputs.m_directionToLight = directionToLight;
        inputs.m_levels = levels;
        if (const auto* meshes = view.m_scene->GetModules().TryFind<MeshSceneModule>())
            inputs.m_staticChanges = meshes->GetStaticChanges();

        PageCache& cache = module.GetPageCache();
        PageFrame frame;
        AddRequestPasses(graph, *view.m_renderQueueUploader, inputs, cache, frame);
        AddTransparentRequestPasses(graph, blackboard, dataBuffer.Get(), cache.m_pageTable.Get());
        AddAllocatePasses(graph, inputs, cache, frame);

        // Each clip level renders as child views between the page allocation and the passes that sample the atlas: static
        // casters into the dirty pages of the static atlas, then dynamic casters into this frame's copy of it.
        for (uint32_t levelIndex = 0; levelIndex < settings.m_levelCount; ++levelIndex)
        {
            FE_FG_SCOPE(graph, Fmt::FixedFormat("StaticLevel{}", levelIndex));
            AddLevelPasses(graph,
                           module,
                           Layer::kStatic,
                           { cache.m_pageTable.Get(),
                             cache.m_staticAtlas.Get(),
                             &frame.m_staticPyramids[levelIndex],
                             DrawTags::Static,
                             kStaticPageFlags,
                             levels[levelIndex].m_pageOrigin,
                             levelIndex });
        }

        AddCopyPagesPass(graph, cache, frame);

        for (uint32_t levelIndex = 0; levelIndex < settings.m_levelCount; ++levelIndex)
        {
            FE_FG_SCOPE(graph, Fmt::FixedFormat("DynamicLevel{}", levelIndex));
            AddLevelPasses(graph,
                           module,
                           Layer::kDynamic,
                           { cache.m_pageTable.Get(),
                             frame.m_atlas.Get(),
                             &frame.m_dynamicPyramids[levelIndex],
                             DrawTags::Dynamic,
                             kDynamicPageFlags,
                             levels[levelIndex].m_pageOrigin,
                             levelIndex });
        }

        SunShadowBinding& binding = blackboard.Add<SunShadowBinding>();
        binding.m_data = graph.GetSRV(dataBuffer.Get());
        binding.m_pageTable = graph.GetSRV(cache.m_pageTable.Get());
        binding.m_atlas = graph.GetSRV(frame.m_atlas.Get());

        Output& output = blackboard.Add<Output>();
        output.m_data = std::move(dataBuffer);
        output.m_pageTable = cache.m_pageTable;
        output.m_atlas = std::move(frame.m_atlas);
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
