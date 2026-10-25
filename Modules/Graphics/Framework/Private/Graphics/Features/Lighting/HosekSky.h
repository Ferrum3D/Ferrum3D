#pragma once
#include <Graphics/Features/Lighting/GlobalLightingSceneModule.h>

namespace FE::Graphics
{
    void PrecomputeHosekSky(const SkyAtmosphereParameters& parameters, Vector3 sunDirection, GlobalLightingData& data);
} // namespace FE::Graphics
