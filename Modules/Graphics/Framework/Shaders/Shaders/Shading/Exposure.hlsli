#pragma once

namespace Shading
{
    // Fixed EV100 12. Input illuminance is lux, and luminance is cd/m^2.
    static const float kExposureEV100 = 12.0f;
    static const float kPreExposure = 1.0f / (1.2f * exp2(kExposureEV100));
    // Keep the solar disc finite in both packed HDR and half-float color targets.
    static const float kMaxStoredLuminance = 65000.0f;
} // namespace Shading
