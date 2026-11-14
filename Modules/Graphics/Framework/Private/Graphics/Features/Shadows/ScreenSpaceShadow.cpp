#include <Graphics/Core/PipelineVariantSet.h>
#include <Graphics/Features/Lighting/GlobalLightingSceneModule.h>
#include <Graphics/Features/Shadows/ScreenSpaceShadow.h>
#include <Graphics/Features/Shadows/SunShadow.h>
#include <Graphics/Passes/DeferredShadingPass.h>
#include <Graphics/Passes/RendererPassCommon.h>
#include <Shaders/Shadows/ScreenSpaceShadow.h>

// The vendored header converts between int and float implicitly.
FE_PUSH_MSVC_WARNING(4244)
#include <ThirdParty/Bend/bend_sss_cpu.h>
FE_POP_MSVC_WARNING()

namespace FE::Graphics::ScreenSpaceShadow
{
    namespace
    {
        FE_COMPUTE_PIPELINE_SET(ScreenSpaceShadowPipeline, "Shaders/Shadows/ScreenSpaceShadow.cs.hlsl");


        //! Whether the view shades its sun in deferred shading with shadows that the pass can complement.
        bool IsEnabled(Core::FrameGraphBlackboard& blackboard)
        {
            SunShadow::ViewModule* const* shadows = blackboard.TryGet<SunShadow::ViewModule*>();
            if (shadows == nullptr || !(*shadows)->m_settings.m_usesScreenSpaceShadow)
                return false;

            if (!blackboard.Contains<SunShadowBinding>() || !blackboard.Contains<DeferredShading::ViewModule*>())
                return false;

            // Shading keeps its ray for orthographic cameras, which the pass is not tuned for.
            const RendererViewData& view = blackboard.Get<RendererViewData>();
            return !blackboard.Contains<ViewColorOverride>() && !view.m_view->IsOrthographic();
        }
    } // namespace


    Rc<Core::Texture> AddLightPass(Core::FrameGraph& graph, Core::Texture* depth, const LightDesc& light)
    {
        const Vector2UInt size = depth->GetDesc().GetSize2D();
        const Vector4 clipPosition = light.m_clipPosition;

        float lightProjection[4] = { clipPosition.x, clipPosition.y, clipPosition.z, clipPosition.w };
        int viewportSize[2] = { int(size.x), int(size.y) };
        int minBounds[2] = { 0, 0 };
        const Bend::DispatchList dispatches =
            Bend::BuildDispatchList(lightProjection, viewportSize, minBounds, viewportSize, false, kScreenShadowWaveSize);
        if (dispatches.DispatchCount == 0)
            return nullptr;

        FE_FG_SCOPE(graph, "ScreenSpaceShadow");

        Rc<Core::Texture> visibility = Core::Texture::Create(graph.GetDevice(), "ScreenShadow", Core::Format::kR8_UNORM, size);
        const float* lightCoordinate = dispatches.LightCoordinate_Shader;

        ScreenShadowConstants constants{};
        constants.m_lightCoordinate =
            PackedVector4F(lightCoordinate[0], lightCoordinate[1], lightCoordinate[2], lightCoordinate[3]);
        constants.m_invDepthSize = Math::Reciprocal(Vector2(size));
        constants.m_depth = graph.GetSRV(depth);
        constants.m_sampler = graph.GetSampler(Core::SamplerState::kPointBorderTransparentBlack);
        constants.m_output = graph.GetUAV(visibility.Get());
        constants.m_surfaceThickness = light.m_surfaceThickness;
        constants.m_shadowContrast = Math::Max(light.m_shadowContrast, 1.0f);

        for (int dispatchIndex = 0; dispatchIndex < dispatches.DispatchCount; ++dispatchIndex)
        {
            const Bend::DispatchData& dispatch = dispatches.Dispatch[dispatchIndex];
            const Vector3UInt waveCount(dispatch.WaveCount[0], dispatch.WaveCount[1], dispatch.WaveCount[2]);

            constants.m_waveOffset = Vector2Int(dispatch.WaveOffset_Shader[0], dispatch.WaveOffset_Shader[1]);
            graph.AddDispatch("ScreenSpaceShadow", ScreenSpaceShadowPipeline::GetPipeline(), constants, waveCount);
        }

        return visibility;
    }


    void AddPasses(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard)
    {
        if (!IsEnabled(blackboard))
            return;

        const RendererViewData& view = blackboard.Get<RendererViewData>();
        const auto* lighting = view.m_scene->GetModules().TryFind<GlobalLightingSceneModule>();
        if (lighting == nullptr || lighting->GetParameters().m_directionalLightIlluminanceLux <= 0.0f)
            return;

        const SunShadow::Settings& settings = blackboard.Get<SunShadow::ViewModule*>()->m_settings;
        const Vector3 directionToLight = SunShadow::GetDirectionToLight(lighting->GetParameters());

        // The rays follow the depth, which is rendered with the jittered projection.
        LightDesc light;
        light.m_clipPosition = Vector4(directionToLight, 0.0f) * blackboard.Get<ViewData>().m_viewProjection;
        light.m_surfaceThickness = settings.m_screenShadowThickness;
        light.m_shadowContrast = settings.m_screenShadowContrast;

        Rc<Core::Texture> visibility = AddLightPass(graph, view.m_mainDepthTarget, light);
        if (visibility == nullptr)
            return;

        blackboard.Add<Output>().m_sunVisibility = std::move(visibility);
    }
} // namespace FE::Graphics::ScreenSpaceShadow
