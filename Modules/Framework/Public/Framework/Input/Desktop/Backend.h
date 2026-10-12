#pragma once
#include <Framework/Application/Core/PlatformEvent.h>
#include <Framework/Input/Core/Backend.h>
#include <festd/vector.h>

namespace FE::Input::Desktop
{
    struct Backend final : Core::Backend
    {
        [[nodiscard]] festd::span<const Core::DeviceKind> EnumerateDevices() const override;
        void Update() override;
        [[nodiscard]] uint64_t GetSampleSequence() const override
        {
            return m_sampleSequence;
        }
        void Update(festd::span<const Framework::Core::PlatformEvent> events, float deltaSeconds = 0, double frameEndSeconds = 0);
        [[nodiscard]] festd::span<const Core::Sample> GetSamples() const override
        {
            return m_samples;
        }
        [[nodiscard]] const Core::KeyboardState& GetKeyboardState() const override
        {
            return m_keyboard;
        }
        [[nodiscard]] const Core::MouseState& GetMouseState() const override
        {
            return m_mouse;
        }

    private:
        festd::vector<Core::Sample> m_samples;
        double m_previousUpdateSeconds = 0;
        uint64_t m_sampleSequence = 0;
        Core::KeyboardState m_keyboard;
        Core::MouseState m_mouse;
        Vector2 m_position{ 0, 0 };
        bool m_hasPosition = false;
    };
} // namespace FE::Input::Desktop
