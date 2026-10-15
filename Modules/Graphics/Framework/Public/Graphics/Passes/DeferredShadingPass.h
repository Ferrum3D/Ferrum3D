#pragma once
#include <Graphics/Core/FrameGraph/FrameGraph.h>
#include <Graphics/Passes/SurfaceWorkBuilder.h>
#include <Graphics/Scene/View.h>

namespace FE::Graphics::DeferredShading
{
    struct PassData final
    {
        Visibility::SurfaceWork* m_work = nullptr;
    };


    struct ViewModule final : public ViewModuleBase
    {
        FE_RTTI("3F99C20A-F2DE-442A-AC4E-930462412DBD");

        explicit ViewModule(View* view);
        ~ViewModule() override;
        void Update(Core::FrameGraphBlackboard& blackboard) override;

    private:
        void DoRelease() override;

        Visibility::SurfaceWork m_work;
    };


    void AddPasses(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard);
} // namespace FE::Graphics::DeferredShading
