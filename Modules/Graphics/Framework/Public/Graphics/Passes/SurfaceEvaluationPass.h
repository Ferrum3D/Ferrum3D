#pragma once
#include <Graphics/Core/FrameGraph/FrameGraph.h>
#include <Graphics/Scene/View.h>

namespace FE::Graphics::SurfaceEvaluation
{
    struct Output final
    {
        Rc<Core::Texture> m_gBufferTargets[4];
        //! UV-space offset to each pixel's previous position; null unless RendererViewData::m_needsMotionVectors.
        Rc<Core::Texture> m_motion;
    };


    //! @brief Evaluate the materials of visible surfaces into the GBuffer targets.
    struct ViewModule final : public ViewModuleBase
    {
        FE_RTTI("D0880186-F009-4F88-896B-916A732C9BCC");

        explicit ViewModule(View* view);
        ~ViewModule() override;
        void Update(Core::FrameGraphBlackboard& blackboard) override;

    private:
        void DoRelease() override;
    };


    void AddPasses(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard);
} // namespace FE::Graphics::SurfaceEvaluation
