#include <Shaders/Shadows/ScreenSpaceShadow.h>

// Writes the fraction of the light that the camera depth leaves visible at every pixel, with Bend Studio's screen space
// shadows. The vendored header is signed, so its configuration is cast from the shared constants.

#define WAVE_SIZE int(kScreenShadowWaveSize)
#define SAMPLE_COUNT int(kScreenShadowSampleCount)
#define HARD_SHADOW_SAMPLES int(kScreenShadowHardSamples)
#define FADE_OUT_SAMPLES int(kScreenShadowFadeSamples)

#include <ThirdParty/Bend/bend_sss_gpu.h>

[[vk::push_constant]] ScreenShadowConstants GConstants;


FE_NUM_THREADS(kScreenShadowWaveSize, 1, 1)
void main(const uint3 group : SV_GroupID, const uint thread : SV_GroupIndex)
{
    DispatchParameters parameters;
    parameters.SetDefaults();
    parameters.SurfaceThickness = GConstants.m_surfaceThickness;
    parameters.ShadowContrast = GConstants.m_shadowContrast;
    parameters.LightCoordinate = GConstants.m_lightCoordinate;
    parameters.WaveOffset = GConstants.m_waveOffset;

    // Reverse Z.
    parameters.FarDepthValue = 0.0f;
    parameters.NearDepthValue = 1.0f;

    parameters.InvDepthTextureSize = GConstants.m_invDepthSize;
    parameters.DepthTexture = GConstants.m_depth.Get();
    parameters.OutputTexture = GConstants.m_output.Get();
    parameters.PointBorderSampler = GConstants.m_sampler.Get();

    WriteScreenSpaceShadow(parameters, int3(group), int(thread));
}
