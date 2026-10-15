#pragma once
#include <Graphics/Core/FrameGraph/FrameGraph.h>
#include <Graphics/Passes/SurfaceWorkBuilder.h>
#include <Graphics/Scene/View.h>

namespace FE::Graphics::SurfaceEvaluation
{
    struct PassData final
    {
        Visibility::SurfaceWork* m_work = nullptr;
    };


    struct Output final
    {
        Rc<Core::Texture> m_gBufferTargets[4];
    };


    struct ViewModule final : public ViewModuleBase
    {
        FE_RTTI("D0880186-F009-4F88-896B-916A732C9BCC");

        explicit ViewModule(View* view);
        ~ViewModule() override;
        void Update(Core::FrameGraphBlackboard& blackboard) override;

    private:
        void DoRelease() override;

        Visibility::SurfaceWork m_work;
    };


    void AddPasses(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard);
} // namespace FE::Graphics::SurfaceEvaluation
