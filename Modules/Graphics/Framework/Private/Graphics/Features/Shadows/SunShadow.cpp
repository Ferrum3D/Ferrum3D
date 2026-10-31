#include <Graphics/Core/PipelineVariantSet.h>
#include <Graphics/Features/Lighting/GlobalLightingSceneModule.h>
#include <Graphics/Features/Shadows/SunShadow.h>
#include <Graphics/Features/Shadows/SunShadowPages.h>
#include <Graphics/Passes/DrawTags.h>
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
        FE_COMPUTE_PIPELINE_SET(ClearPageTablePipeline, "Shaders/Shadows/ClearPageTable.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(MarkPagesPipeline, "Shaders/Shadows/MarkPages.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(AllocatePagesPipeline, "Shaders/Shadows/AllocatePages.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(ClearAtlasPagesPipeline, "Shaders/Shadows/ClearAtlasPages.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(SunShadowDebugPipeline, "Shaders/Shadows/SunShadowDebug.cs.hlsl");

        constexpr uint32_t kDebugModes[] = { kSunShadowDebugLevel, kSunShadowDebugPage, kSunShadowDebugFactor };


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
    } // namespace


    PageResources PageResources::Create(Core::Device* device, const uint32_t levelCount)
    {
        PageResources resources;
        resources.m_pageCount = levelCount * kSunShadowLevelPageCount;
        resources.m_pageTable = Core::Buffer::CreateStructured<uint32_t>(device, "SunShadowPageTable", resources.m_pageCount);
        resources.m_counters = Core::Buffer::CreateStructured<uint32_t>(device, "SunShadowCounters", kSunShadowCounterCount);
        resources.m_atlas = Core::Texture::Create(device,
                                                  "SunShadowAtlas",
                                                  Core::Format::kR32_UINT,
                                                  Vector2UInt(kSunShadowAtlasSize, kSunShadowAtlasSize));
        return resources;
    }


    void AddPagePasses(Core::FrameGraph& graph, const PageInputs& inputs, const PageResources& resources)
    {
        FE_PROFILER_ZONE();

        auto* clear = graph.AllocatePassData<SunShadowClearPassDesc>();
        clear->m_pipeline = ClearPageTablePipeline::GetPipeline();
        clear->m_constants.m_pageTable = graph.GetUAV(resources.m_pageTable.Get());
        clear->m_constants.m_counters = graph.GetUAV(resources.m_counters.Get());
        clear->m_constants.m_pageCount = resources.m_pageCount;
        graph.AddDispatchPass("ClearPageTable", clear, Math::CeilDivide(resources.m_pageCount, kSunShadowThreadCount));

        const Vector2UInt resolution = inputs.m_depth->GetDesc().GetSize2D();
        auto* mark = graph.AllocatePassData<SunShadowMarkPassDesc>();
        mark->m_pipeline = MarkPagesPipeline::GetPipeline();
        mark->m_constants.m_data = graph.GetSRV(inputs.m_data);
        mark->m_constants.m_view = graph.GetSRV(inputs.m_view);
        mark->m_constants.m_depth = graph.GetSRV(inputs.m_depth);
        mark->m_constants.m_pageTable = graph.GetUAV(resources.m_pageTable.Get());
        mark->m_constants.m_resolution = resolution;
        graph.AddDispatchPass("MarkPages", mark, Math::CeilDivide(resolution, kSunShadowTileSize));

        auto* allocate = graph.AllocatePassData<SunShadowAllocatePassDesc>();
        allocate->m_pipeline = AllocatePagesPipeline::GetPipeline();
        allocate->m_constants.m_pageTable = graph.GetUAV(resources.m_pageTable.Get());
        allocate->m_constants.m_counters = graph.GetUAV(resources.m_counters.Get());
        allocate->m_constants.m_pageCount = resources.m_pageCount;
        graph.AddDispatchPass("AllocatePages", allocate, Math::CeilDivide(resources.m_pageCount, kSunShadowThreadCount));

        auto* clearAtlas = graph.AllocatePassData<SunShadowClearAtlasPassDesc>();
        clearAtlas->m_pipeline = ClearAtlasPagesPipeline::GetPipeline();
        clearAtlas->m_constants.m_counters = graph.GetSRV(resources.m_counters.Get());
        clearAtlas->m_constants.m_atlas = graph.GetUAV(resources.m_atlas.Get());
        graph.AddDispatchPass("ClearAtlasPages", clearAtlas, kSunShadowPhysicalPageCount);
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

        levels.clear();
        for (uint32_t levelIndex = 0; levelIndex < settings.m_levelCount; ++levelIndex)
        {
            // Moving the level in whole pages keeps every texel at the same world position while the camera moves.
            const float size = settings.m_level0Size * float(1u << levelIndex);
            const float pageSize = size / kSunShadowLevelPagesPerAxis;
            const Vector3 snapped(Math::Floor(lightSpaceCamera.x / pageSize) * pageSize,
                                  Math::Floor(lightSpaceCamera.y / pageSize) * pageSize,
                                  lightSpaceCamera.z - settings.m_depthRange * 0.5f);

            LevelSetup& level = levels.emplace_back();
            level.m_camera = Transform::Create(Math::Rotate(snapped, rotation), rotation, 1.0f);
            level.m_size = size;
            level.m_texelSize = size / kSunShadowVirtualSize;

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
            data.m_levels[levelIndex].m_texelSize = PackedVector4F(levels[levelIndex].m_texelSize, 0.0f, 0.0f, 0.0f);
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

        while (m_levelViews.size() < m_settings.m_levelCount)
        {
            Rc<View> levelView = m_view->GetScene()->CreateChildView();
            levelView->GetModules().Add<ShadowAtlas::ViewModule>();
            m_levelViews.push_back(std::move(levelView));
        }

        m_levelViews.resize(m_settings.m_levelCount);
        blackboard.Add<ViewModule*>(this);

        if (m_debugMode == kInvalidIndex)
            return;

        blackboard.Add<DebugPassData>(kDebugModes[m_debugMode]);
        blackboard.Add<ViewColorOverride>(true);
    }


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

        festd::fixed_vector<LevelSetup, kSunShadowMaxLevelCount> levels;
        CalculateLevels(settings, camera.m_position, GetDirectionToLight(lighting->GetParameters()), levels);
        for (uint32_t levelIndex = 0; levelIndex < levels.size(); ++levelIndex)
        {
            const LevelSetup& level = levels[levelIndex];
            View& levelView = *module.GetLevelView(levelIndex);
            levelView.SetCameraTransform(level.m_camera);
            levelView.SetOrthographicProjection(level.m_size, level.m_size, 0.0f, settings.m_depthRange);
        }

        const SunShadowData data = BuildShadowData(settings, levels, camera);
        const PageResources resources = PageResources::Create(graph.GetDevice(), settings.m_levelCount);
        Rc<Core::Buffer> dataBuffer = Core::Buffer::CreateStructured<SunShadowData>(graph.GetDevice(), "SunShadowData", 1);
        FE_Verify(view.m_renderQueueUploader->Upload(graph, dataBuffer.Get(), data));

        PageInputs inputs;
        inputs.m_data = dataBuffer.Get();
        inputs.m_view = view.m_constants.Get();
        inputs.m_depth = view.m_mainDepthTarget;
        AddPagePasses(graph, inputs, resources);

        // Each clip level renders as a child view between the page allocation and the passes that sample the atlas.
        for (uint32_t levelIndex = 0; levelIndex < settings.m_levelCount; ++levelIndex)
        {
            FE_FG_SCOPE(graph, Fmt::FixedFormat("Level{}", levelIndex));
            View* levelView = module.GetLevelView(levelIndex);
            ShadowAtlas::ViewModule& atlas = levelView->GetModules().Find<ShadowAtlas::ViewModule>();
            atlas.m_inputs = { resources.m_pageTable.Get(), resources.m_atlas.Get(), levelIndex };
            Renderer::Get().AddViewPasses(graph, { levelView, Vector2UInt(kSunShadowVirtualSize, kSunShadowVirtualSize) });
            atlas.m_inputs = {};
        }

        SunShadowBinding& binding = blackboard.Add<SunShadowBinding>();
        binding.m_data = graph.GetSRV(dataBuffer.Get());
        binding.m_pageTable = graph.GetSRV(resources.m_pageTable.Get());
        binding.m_atlas = graph.GetSRV(resources.m_atlas.Get());
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
        // Shadow views render once per frame without occlusion culling.
        Culling::PassData& culling = blackboard.Add<Culling::PassData>();
        culling.m_settings.m_instanceOcclusionCulling = false;
        culling.m_settings.m_meshletOcclusionCulling = false;
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
        pass.m_drawTag = DrawTags::Visibility;
        // Only materials with a ShadowDepth technique cast sun shadows.
        pass.m_techniqueRole = Env::Name("ShadowDepth");
        pass.m_phase = SceneRenderPhase::kSingle;
        pass.m_passData = rasterBuffer.Get();
        view.m_scene->GetModules().ForEachActive([&](SceneModuleBase& module) {
            module.AddRenderPasses(graph, blackboard, *view.m_renderQueueUploader, pass);
        });
    }
} // namespace FE::Graphics::ShadowAtlas
