#include <Graphics/Core/PipelineVariantSet.h>
#include <Graphics/Passes/RendererPassCommon.h>
#include <Graphics/Passes/VisibilityDebug.h>
#include <Graphics/Passes/VisibilityPass.h>
#include <Graphics/Scene/SurfaceEvaluation.h>
#include <Shaders/Passes/Visibility/Debug.h>

namespace FE::Graphics::Visibility
{
    namespace
    {
        FE_COMPUTE_PIPELINE_SET(InitializeDebugPipeline, "Shaders/Passes/Visibility/InitializeDebug.cs.hlsl");

#define FE_DEBUG_MODE_VALUE(name, text, value) DebugMode::k##name,
        constexpr DebugMode kModes[] = { FE_VISIBILITY_DEBUG_MODES(FE_DEBUG_MODE_VALUE) };
#undef FE_DEBUG_MODE_VALUE


        festd::span<const Env::Name> GetModeNames()
        {
            // Names are interned on first use, after the environment is initialized.
#define FE_DEBUG_MODE_NAME(name, text, value) Env::Name(text),
            static const Env::Name kModeNames[] = { FE_VISIBILITY_DEBUG_MODES(FE_DEBUG_MODE_NAME) };
#undef FE_DEBUG_MODE_NAME
            return kModeNames;
        }
    } // namespace


    void InitializeDebugColor(Core::FrameGraph& graph, Core::Texture* target)
    {
        auto* pass = graph.AllocatePassData<DebugPassDesc>();
        pass->m_pipeline = InitializeDebugPipeline::GetPipeline();
        pass->m_constants.m_output = graph.GetUAV(target);
        pass->m_constants.m_resolution = target->GetDesc().GetSize2D();
        graph.AddDispatchPass("InitializeDebugColor", pass, Math::CeilDivide(pass->m_constants.m_resolution, 8u));
    }


    DebugViewModule::DebugViewModule(View* view)
        : ViewModuleBase(view)
    {
    }


    DebugViewModule::~DebugViewModule() = default;


    void DebugViewModule::DoRelease()
    {
        Memory::DefaultDelete(this);
    }


    festd::span<const Env::Name> DebugViewModule::GetDebugModes() const
    {
        return GetModeNames();
    }


    void DebugViewModule::SetDebugMode(const uint32_t modeIndex)
    {
        FE_Assert(modeIndex == kInvalidIndex || modeIndex < festd::size(kModes));
        m_modeIndex = modeIndex;
    }


    void DebugViewModule::Update(Core::FrameGraphBlackboard& blackboard)
    {
        if (m_modeIndex == kInvalidIndex)
            return;

        blackboard.Add<DebugPassData>(kModes[m_modeIndex]);
        blackboard.Add<ViewColorOverride>(false);
    }


    void AddDebugPasses(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard)
    {
        const DebugPassData* data = blackboard.TryGet<DebugPassData>();
        const Output* visibility = blackboard.TryGet<Output>();
        if (data == nullptr || visibility == nullptr)
            return;

        FE_FG_SCOPE(graph, "VisibilityDebug");

        RendererViewData& view = blackboard.Get<RendererViewData>();
        const Vector2UInt resolution = view.m_mainColorTarget->GetDesc().GetSize2D();
        InitializeDebugColor(graph, view.m_mainColorTarget);
        view.m_isMainColorWritten = true;

        // Reconstruction belongs to the object producer, which supplies its debug shader and data.
        for (const SurfaceEvaluationBatch& batch : blackboard.Get<SurfaceEvaluationInputs>())
        {
            auto* debug = graph.AllocatePassData<DebugPassDesc>();
            debug->m_pipeline = batch.m_visibilityDebugPipeline;
            debug->m_constants.m_view = graph.GetSRV(batch.m_objectData.Get());
            debug->m_constants.m_visibility = graph.GetSRV(visibility->m_target.Get());
            debug->m_constants.m_depth = graph.GetSRV(view.m_mainDepthTarget);
            debug->m_constants.m_output = graph.GetUAV(view.m_mainColorTarget);
            debug->m_constants.m_resolution = resolution;
            debug->m_constants.m_mode = festd::to_underlying(data->m_mode);
            graph.AddDispatchPass("VisibilityReconstruction", debug, Math::CeilDivide(resolution, 8u));
        }
    }
} // namespace FE::Graphics::Visibility
