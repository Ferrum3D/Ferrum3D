#pragma once
#include <Shaders/Base/Base.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics)
    //! The screen space shadow pass follows Bend Studio's technique: the threads of one group are consecutive pixels of one
    //! ray towards the light, so they share the depth they read through LDS.
    FE_CONSTEXPR uint32_t kScreenShadowWaveSize = 64;
    //! Pixels towards the light that can shadow a pixel. The last kScreenShadowFadeSamples of them fade out, and the first
    //! kScreenShadowHardSamples shadow fully on their own to keep contact with the caster.
    FE_CONSTEXPR uint32_t kScreenShadowSampleCount = 60;
    FE_CONSTEXPR uint32_t kScreenShadowHardSamples = 4;
    FE_CONSTEXPR uint32_t kScreenShadowFadeSamples = 8;


    //! One of the dispatches that together cover the view; they differ only in m_waveOffset.
    struct ScreenShadowConstants final
    {
        //! Pixel position, depth and sign of w of the light projected by the camera, see Bend::BuildDispatchList.
        float4 m_lightCoordinate;
        int2 m_waveOffset;
        float2 m_invDepthSize;
        Texture2DDescriptor<float> m_depth;
        //! Point filtered, with the far depth beyond the borders of the depth.
        SamplerDescriptor m_sampler;
        RWTexture2DDescriptor<float> m_output;
        float m_surfaceThickness;
        float m_shadowContrast;

        FE_RTTI_Reflect("6D39B5EE-3C44-461E-A183-CF9063EC9AB9");
    };

FE_HOST_END_NAMESPACE
