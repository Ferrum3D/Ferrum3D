#pragma once
#include <Graphics/Scene/View.h>
#include <Shaders/Passes/Visibility/DebugModes.h>

namespace FE::Graphics::GBufferDebug
{
    //! @brief Published by an active ViewModule.
    struct PassData final
    {
        Visibility::DebugMode m_mode = Visibility::DebugMode::kSurfaceColor;
    };


    //! @brief Replace the lit color with GBuffer contents and surface classification diagnostics.
    struct ViewModule final : public ViewModuleBase
    {
        FE_RTTI("EC61E888-A825-4805-8320-BB01D72013A0");

        explicit ViewModule(View* view);
        ~ViewModule() override;

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


    //! @brief Visualize the selected GBuffer diagnostic into the main color target.
    void AddPasses(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard);
} // namespace FE::Graphics::GBufferDebug
