#pragma once
#include <Graphics/Scene/View.h>

namespace FE::Graphics::Tonemap
{
    //! @brief Map the HDR main color of a scene view onto its presentation output.
    struct ViewModule final : public ViewModuleBase
    {
        FE_RTTI("93BC5B9C-7B1C-4829-BFC7-98475F8D91BD");

        explicit ViewModule(View* view);
        ~ViewModule() override;
        void Update(Core::FrameGraphBlackboard& blackboard) override;

    private:
        void DoRelease() override;
    };


    //! @brief Initialize the main color target when no earlier pass wrote it.
    void AddPasses(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard);

    //! @brief Tonemap the main color target into the acquired ViewOutput.
    void AddOutputPasses(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard);
} // namespace FE::Graphics::Tonemap
