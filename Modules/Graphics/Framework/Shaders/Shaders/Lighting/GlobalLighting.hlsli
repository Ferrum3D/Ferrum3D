#pragma once
#include <Shaders/Lighting/GlobalLighting.h>

GlobalLightingData LoadGlobalLighting(ConstantsPtr<GlobalLightingData> lighting)
{
    if (!lighting.IsValid())
        return (GlobalLightingData)0;

    return lighting.Load();
}
