#pragma once
#include <Graphics/Scene/View.h>
#include <Shaders/Passes/Visibility/DebugModes.h>

namespace FE::Graphics::Visibility
{
    void InitializeDebugColor(Core::FrameGraph& graph, Core::Texture* target);


    //! @brief Published by an active DebugViewModule.
    struct DebugPassData final
    {
        DebugMode m_mode = DebugMode::kVisibility;
    };


    //! @brief Replace the lit color with visibility reconstruction diagnostics supplied by each object producer.
    struct DebugViewModule final : public ViewModuleBase
    {
        FE_RTTI("CF238FED-C446-4FDC-A577-44E5EDCDD425");

        explicit DebugViewModule(View* view);
        ~DebugViewModule() override;

        void Update(Core::FrameGraphBlackboard& blackboard) override;

        [[nodiscard]] festd::span<const Env::Name> GetDebugModes() const override;
        void SetDebugMode(uint32_t modeIndex) override;

        [[nodiscard]] uint32_t GetDebugMode() const override
        {
            return m_modeIndex;
        }

    private:
        void DoRelease() override;

        uint32_t m_modeIndex = kInvalidIndex;
    };


    //! @brief Reconstruct the selected diagnostic from the visibility buffer into the main color target.
    void AddDebugPasses(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard);
} // namespace FE::Graphics::Visibility
