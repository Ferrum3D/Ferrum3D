#pragma once
#include <Shaders/Deferred/Tiling.h>
#include <Shaders/Lighting/ViewLighting.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics::Visibility)
    struct DeferredConstants final
    {
        ConstantsPtr<ViewData> m_view;
        ConstantsPtr<ViewLighting> m_lighting;
        Texture2DDescriptor<uint32_t> m_g0;
        Texture2DDescriptor<uint32_t> m_g1;
        Texture2DDescriptor<uint32_t> m_g2;
        Texture2DDescriptor<uint32_t> m_g3;
        Texture2DDescriptor<float> m_depth;
        RWTexture2DDescriptor<float3> m_output;
        SurfaceWorkBinding m_work;
        uint32_t m_shaderCount;
        //! Screen space shadow of the sun, see ScreenSpaceShadow::Output; invalid in views without one.
        Texture2DDescriptor<float> m_sunScreenShadow;
        FE_RTTI_Reflect("B2A55DB1-8534-475D-A6E0-5C4C025861FD");
    };

FE_HOST_END_NAMESPACE
