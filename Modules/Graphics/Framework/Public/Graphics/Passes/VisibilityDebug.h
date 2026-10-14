#pragma once
#include <festd/string.h>

namespace FE::Graphics::Visibility
{
    enum class DebugMode : uint32_t
    {
        kForward,
        kVisibility,
        kInstance,
        kMeshlet,
        kTriangle,
        kUV,
        kNormal,
        kTangent,
        kDerivatives,
        kDepthError,
    };


    [[nodiscard]] DebugMode ParseDebugMode(festd::string_view value);
} // namespace FE::Graphics::Visibility
