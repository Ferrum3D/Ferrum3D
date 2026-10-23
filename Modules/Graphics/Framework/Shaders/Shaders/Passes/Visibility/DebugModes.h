#pragma once
#include <Shaders/Base/Base.h>

// The lists supply the public enum, shader values and the debug mode names of their view modules.
#define FE_VISIBILITY_DEBUG_MODES(MODE)                                                                                          \
    MODE(Visibility, "visibility", 1)                                                                                            \
    MODE(Instance, "instance", 2)                                                                                                \
    MODE(Meshlet, "meshlet", 3)                                                                                                  \
    MODE(Triangle, "triangle", 4)                                                                                                \
    MODE(UV, "uv", 5)                                                                                                            \
    MODE(Normal, "normal", 6)                                                                                                    \
    MODE(Tangent, "tangent", 7)                                                                                                  \
    MODE(Derivatives, "derivatives", 8)                                                                                          \
    MODE(DepthError, "depth-error", 9) MODE(Lod, "lod", 17)

#define FE_SURFACE_DEBUG_MODES(MODE)                                                                                             \
    MODE(EvaluationTile, "evaluation-tile", 10)                                                                                  \
    MODE(EvaluationGroup, "evaluation-group", 11)                                                                                \
    MODE(SurfaceColor, "surface-color", 12)                                                                                      \
    MODE(SurfaceNormal, "surface-normal", 13)                                                                                    \
    MODE(SurfaceProperties, "surface-properties", 14)                                                                            \
    MODE(SurfaceEmissive, "surface-emissive", 15)                                                                                \
    MODE(SurfaceRoute, "surface-route", 16)

FE_HOST_BEGIN_NAMESPACE(FE::Graphics::Visibility)
#if FE_HOST
    enum class DebugMode : uint32_t
    {
#    define FE_DEBUG_MODE(name, text, value) k##name = value,
        FE_VISIBILITY_DEBUG_MODES(FE_DEBUG_MODE) FE_SURFACE_DEBUG_MODES(FE_DEBUG_MODE)
#    undef FE_DEBUG_MODE
    };
#else
    namespace DebugMode
    {
#    define FE_DEBUG_MODE(name, text, value) FE_CONSTEXPR uint32_t k##name = value;
        FE_VISIBILITY_DEBUG_MODES(FE_DEBUG_MODE)
        FE_SURFACE_DEBUG_MODES(FE_DEBUG_MODE)
#    undef FE_DEBUG_MODE
    } // namespace DebugMode
#endif
FE_HOST_END_NAMESPACE
