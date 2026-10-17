#include <Graphics/Core/PipelineVariantSet.h>
#include <Graphics/Passes/VisibilityDebug.h>
#include <Shaders/Passes/Visibility/Debug.h>

namespace FE::Graphics::Visibility
{
    namespace
    {
        FE_COMPUTE_PIPELINE_SET(InitializeDebugPipeline, "Shaders/Passes/Visibility/InitializeDebug.cs.hlsl");
    } // namespace


    DebugMode ParseDebugMode(const festd::string_view value)
    {
#define FE_DEBUG_MODE(name, text, number)                                                                                        \
    if (value == text)                                                                                                           \
        return DebugMode::k##name;
        FE_VISIBILITY_DEBUG_MODES(FE_DEBUG_MODE)
#undef FE_DEBUG_MODE

        FE_Assert(false, "Unknown visibility debug mode");
        return DebugMode::kDeferred;
    }


    bool NeedsSurfaceEvaluation(const DebugMode mode)
    {
        switch (mode)
        {
        case DebugMode::kEvaluationTile:
        case DebugMode::kEvaluationGroup:
        case DebugMode::kSurfaceColor:
        case DebugMode::kSurfaceNormal:
        case DebugMode::kSurfaceProperties:
        case DebugMode::kSurfaceEmissive:
        case DebugMode::kSurfaceRoute:
        case DebugMode::kDeferred:
            return true;

        default:
            return false;
        }
    }


    bool IsReconstructionDebug(const DebugMode mode)
    {
        return !NeedsSurfaceEvaluation(mode);
    }


    bool NeedsDeferredShading(const DebugMode mode)
    {
        return mode == DebugMode::kDeferred;
    }


    void InitializeDebugColor(Core::FrameGraph& graph, Core::Texture* target)
    {
        auto* pass = graph.AllocatePassData<DebugPassDesc>();
        pass->m_pipeline = InitializeDebugPipeline::GetPipeline();
        pass->m_constants.m_output = graph.GetUAV(target);
        pass->m_constants.m_resolution = target->GetDesc().GetSize2D();
        graph.AddDispatchPass("InitializeDebugColor", pass, Math::CeilDivide(pass->m_constants.m_resolution, 8u));
    }
} // namespace FE::Graphics::Visibility
