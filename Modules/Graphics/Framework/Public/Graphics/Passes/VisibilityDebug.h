#pragma once
#include <festd/string.h>

namespace FE::Graphics::Visibility
{
    enum class DebugMode : uint32_t
    {
        kVisibility = 1,
        kInstance,
        kMeshlet,
        kTriangle,
        kUV,
        kNormal,
        kTangent,
        kDerivatives,
        kDepthError,
        kEvaluationTile,
        kEvaluationGroup,
        kSurfaceColor,
        kSurfaceNormal,
        kSurfaceProperties,
        kSurfaceEmissive,
        kSurfaceRoute,
        kDeferred,
    };


    [[nodiscard]] DebugMode ParseDebugMode(festd::string_view value);
} // namespace FE::Graphics::Visibility
