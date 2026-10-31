#pragma once
#include <Graphics/Scene/View.h>

namespace FE::Graphics::ChildViewProbe
{
    //! Renders a child view from the pass setup of its parent and records what the child's setup published.
    struct ViewModule final : public ViewModuleBase
    {
        FE_RTTI("8959836C-EC57-45A5-9381-DF571ABA3388");

        Rc<View> m_child;
        Vector2UInt m_childResolution = Vector2UInt::kZero;
        Vector2UInt m_childVisibilitySize = Vector2UInt::kZero;
        const View* m_childBlackboardView = nullptr;
        bool m_isChildOrthographic = false;
        bool m_isParentBlackboardSeparate = false;

        explicit ViewModule(View* view);

        void Update(Core::FrameGraphBlackboard& blackboard) override;
        void AddRenderPasses(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard) override;

    private:
        void DoRelease() override;
    };
} // namespace FE::Graphics::ChildViewProbe
