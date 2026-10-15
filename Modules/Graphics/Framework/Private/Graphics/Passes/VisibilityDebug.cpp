#include <Graphics/Passes/VisibilityDebug.h>

namespace FE::Graphics::Visibility
{
    DebugMode ParseDebugMode(const festd::string_view value)
    {
        constexpr festd::string_view names[] = { "",
                                                 "visibility",
                                                 "instance",
                                                 "meshlet",
                                                 "triangle",
                                                 "uv",
                                                 "normal",
                                                 "tangent",
                                                 "derivatives",
                                                 "depth-error",
                                                 "evaluation-tile",
                                                 "evaluation-group",
                                                 "surface-color",
                                                 "surface-normal",
                                                 "surface-properties",
                                                 "surface-emissive",
                                                 "surface-route",
                                                 "deferred" };
        for (uint32_t index = 1; index < festd::size(names); ++index)
        {
            if (value == names[index])
                return static_cast<DebugMode>(index);
        }

        FE_Assert(false, "Unknown visibility debug mode");
        return DebugMode::kDeferred;
    }


} // namespace FE::Graphics::Visibility
