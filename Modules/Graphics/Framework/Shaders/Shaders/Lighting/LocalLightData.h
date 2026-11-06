#pragma once
#include <Shaders/Base/Base.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics)
    FE_CONSTEXPR float kLocalLightConeAngleScale = 65534.0f / 1.5707963267948966f;
    FE_CONSTEXPR uint32_t kLocalPointLightCone = 0xffffffffu;


    //! What shading needs of a light besides its sphere, which culling tests far more often.
    struct LocalLightShading final
    {
        uint32_t m_color;
        float m_colorMultiplier;
        uint32_t m_direction;
        //! Inner/outer half-angles as two UNORM16 values; 0xffffffff denotes a point light.
        uint32_t m_coneAngles;
    };


    struct LocalLightData final
    {
        float4 m_positionRange;
        uint32_t m_color;
        float m_colorMultiplier;
        uint32_t m_direction;
        //! Inner/outer half-angles as two UNORM16 values; 0xffffffff denotes a point light.
        uint32_t m_coneAngles;
    };
FE_HOST_END_NAMESPACE
