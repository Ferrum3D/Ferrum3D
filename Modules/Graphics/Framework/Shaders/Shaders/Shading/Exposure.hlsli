#pragma once

namespace Shading
{
    // Fixed EV100 12. Input illuminance is lux, and luminance is cd/m^2.
    static const float kExposureEV100 = 12.0f;
    static const float kPreExposure = 1.0f / (1.2f * exp2(kExposureEV100));
} // namespace Shading
