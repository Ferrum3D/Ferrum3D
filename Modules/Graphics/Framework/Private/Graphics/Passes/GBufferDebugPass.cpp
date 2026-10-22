#include <Graphics/Core/PipelineVariantSet.h>
#include <Graphics/Passes/GBufferDebugPass.h>
#include <Graphics/Passes/RendererPassCommon.h>
#include <Graphics/Passes/SurfaceEvaluationPass.h>
#include <Graphics/Passes/VisibilityDebug.h>
#include <Graphics/Passes/VisibilityPass.h>
#include <Graphics/Scene/SurfaceEvaluation.h>
#include <Shaders/Passes/Visibility/Debug.h>

namespace FE::Graphics::GBufferDebug
{
    using Visibility::DebugMode;

    namespace
    {
        FE_COMPUTE_PIPELINE_SET(SurfaceDebugPipeline, "Shaders/Deferred/SurfaceDebug.cs.hlsl");

#define FE_DEBUG_MODE_VALUE(name, text, value) DebugMode::k##name,
        constexpr DebugMode kModes[] = { FE_SURFACE_DEBUG_MODES(FE_DEBUG_MODE_VALUE) };
#undef FE_DEBUG_MODE_VALUE


        festd::span<const Env::Name> GetModeNames()
        {
            // Names are interned on first use, after the environment is initialized.
#define FE_DEBUG_MODE_NAME(name, text, value) Env::Name(text),
            static const Env::Name kModeNames[] = { FE_SURFACE_DEBUG_MODES(FE_DEBUG_MODE_NAME) };
#undef FE_DEBUG_MODE_NAME
            return kModeNames;
        }


        void AddGroupDebugPasses(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard,
                                 const Visibility::DebugPassDesc& debug)
        {
            // Evaluation groups are producer-specific, so each object producer supplies its own classifier.
            for (const SurfaceEvaluationBatch& batch : blackboard.Get<SurfaceEvaluationInputs>())
            {
                auto* groupDebug = graph.AllocatePassData<Visibility::DebugPassDesc>();
                *groupDebug = debug;
                groupDebug->m_pipeline = batch.m_surfaceDebugPipeline;
                groupDebug->m_constants.m_view = graph.GetSRV(batch.m_objectData.Get());
                graph.AddDispatchPass("SurfaceGroupDebug", groupDebug, Math::CeilDivide(debug.m_constants.m_resolution, 8u));
            }
        }
    } // namespace


    ViewModule::ViewModule(View* view)
        : ViewModuleBase(view)
    {
    }


    ViewModule::~ViewModule() = default;


    void ViewModule::DoRelease()
    {
        Memory::DefaultDelete(this);
    }


    festd::span<const Env::Name> ViewModule::GetDebugModes() const
    {
        return GetModeNames();
    }


    void ViewModule::SetDebugMode(const uint32_t modeIndex)
    {
        FE_Assert(modeIndex == kInvalidIndex || modeIndex < festd::size(kModes));
        m_modeIndex = modeIndex;
    }


    void ViewModule::Update(Core::FrameGraphBlackboard& blackboard)
    {
        if (m_modeIndex == kInvalidIndex)
            return;

        blackboard.Add<PassData>(kModes[m_modeIndex]);
        blackboard.Add<ViewColorOverride>(true);
    }


    void AddPasses(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard)
    {
        const PassData* data = blackboard.TryGet<PassData>();
        const SurfaceEvaluation::Output* surfaces = blackboard.TryGet<SurfaceEvaluation::Output>();
        if (data == nullptr || surfaces == nullptr)
            return;

        FE_FG_SCOPE(graph, "GBufferDebug");

        RendererViewData& view = blackboard.Get<RendererViewData>();
        const auto& targets = surfaces->m_gBufferTargets;
        Core::Texture* visibility = blackboard.Get<Visibility::Output>().m_target.Get();
        const Vector2UInt resolution = visibility->GetDesc().GetSize2D();
        const DebugMode mode = data->m_mode;

        auto* debug = graph.AllocatePassData<Visibility::DebugPassDesc>();
        debug->m_pipeline = SurfaceDebugPipeline::GetPipeline();
        debug->m_constants.m_g0 = graph.GetSRV(targets[0].Get());
        debug->m_constants.m_g1 = graph.GetSRV(targets[1].Get());
        debug->m_constants.m_g2 = graph.GetSRV(targets[2].Get());
        debug->m_constants.m_g3 = graph.GetSRV(targets[3].Get());
        debug->m_constants.m_visibility = graph.GetSRV(visibility);
        debug->m_constants.m_output = graph.GetUAV(view.m_mainColorTarget);
        debug->m_constants.m_resolution = resolution;
        debug->m_constants.m_mode = festd::to_underlying(mode);
        view.m_isMainColorWritten = true;

        if (mode != DebugMode::kEvaluationGroup)
        {
            graph.AddDispatchPass("GBufferDebug", debug, Math::CeilDivide(resolution, 8u));
            return;
        }

        // Group shaders only write covered pixels of their own producer.
        Visibility::InitializeDebugColor(graph, view.m_mainColorTarget);
        AddGroupDebugPasses(graph, blackboard, *debug);
    }
} // namespace FE::Graphics::GBufferDebug
