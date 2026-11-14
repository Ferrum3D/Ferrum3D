#pragma once
#include <Shaders/Base/Base.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics)
    namespace Shading
    {
        // Fixed EV100 12. Input illuminance is lux, and luminance is cd/m^2.
        FE_CONSTEXPR float kExposureEV100 = 12.0f;
        // 2^kExposureEV100, spelled out because the host cannot evaluate exp2 in a constant expression.
        FE_CONSTEXPR float kExposureEV100Scale = 4096.0f;
        FE_CONSTEXPR float kExposure = 1.0f / (1.2f * kExposureEV100Scale);
        // Scene color is stored with the whole exposure applied; the two diverge once exposure adapts.
        FE_CONSTEXPR float kPreExposure = kExposure;
        // Keep the solar disc finite in both packed HDR and half-float color targets.
        FE_CONSTEXPR float kMaxStoredLuminance = 65000.0f;
    } // namespace Shading
FE_HOST_END_NAMESPACE
