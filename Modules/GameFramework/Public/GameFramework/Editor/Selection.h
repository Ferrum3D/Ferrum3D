#pragma once
#include <Graphics/Core/Fence.h>
#include <Graphics/Scene/View.h>

namespace FE::Editor
{
    //! Visibility-based picking and selected mesh silhouettes, installed only on editor camera views.
    struct SelectionViewModule final : Graphics::ViewModuleBase
    {
        FE_RTTI("07C1627E-C90B-4D1B-AC84-A405A43CDB94");

        explicit SelectionViewModule(Graphics::View* view);

        void Initialize(Graphics::Core::PipelineFactory* factory);

        void Update(Graphics::Core::FrameGraphBlackboard& blackboard) override;
        void AddRenderPasses(Graphics::Core::FrameGraph& graph, Graphics::Core::FrameGraphBlackboard& blackboard) override;

        void SetSelection(festd::span<const uint32_t> objects);

        void RequestPick(Vector2UInt pixel);

        //! Call only after joining CPU render work; a GPU fence protects the mapped result.
        bool ConsumePick(uint32_t& object);

        [[nodiscard]] bool IsPickPending() const
        {
            return m_pickPending;
        }

    private:
        void DoRelease() override;

        festd::vector<uint32_t> m_selected;
        Graphics::Core::ComputePipeline* m_pipeline = nullptr;
        Rc<Graphics::Core::Buffer> m_readback;
        Graphics::Core::FenceSyncPoint m_pickFence;
        Vector2UInt m_pickPosition{ 0, 0 };
        bool m_pickRequested = false;
        bool m_pickPending = false;
    };
} // namespace FE::Editor
