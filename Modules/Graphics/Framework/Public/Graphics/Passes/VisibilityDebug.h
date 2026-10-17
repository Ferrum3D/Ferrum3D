#pragma once
#include <Shaders/Passes/Visibility/DebugModes.h>
#include <festd/string.h>

namespace FE::Graphics::Visibility
{
    [[nodiscard]] DebugMode ParseDebugMode(festd::string_view value);
    [[nodiscard]] bool IsReconstructionDebug(DebugMode mode);
    [[nodiscard]] bool NeedsSurfaceEvaluation(DebugMode mode);
    [[nodiscard]] bool NeedsDeferredShading(DebugMode mode);
    void InitializeDebugColor(Core::FrameGraph& graph, Core::Texture* target);
} // namespace FE::Graphics::Visibility
