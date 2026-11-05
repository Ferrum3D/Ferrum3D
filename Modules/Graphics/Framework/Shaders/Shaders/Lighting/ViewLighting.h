#pragma once
#include <Shaders/Lighting/GlobalLighting.h>
#include <Shaders/Lighting/LocalLighting.h>
#include <Shaders/Shadows/SunShadow.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics)
    //! Everything the passes that shade surfaces of a view need to light them, created after the shadow passes of the view.
    struct ViewLighting final
    {
        ConstantsPtr<GlobalLightingData> m_global;
        LocalLightingBinding m_local;
        SunShadowBinding m_sunShadow;
        FE_RTTI_Reflect("CB8CA482-132A-49CF-B3D5-9D388B626D20");
    };
FE_HOST_END_NAMESPACE
