#pragma once
#include <Core/Math/Vector2.h>
#include <EASTL/bitset.h>
#include <Framework/Input/Core/Keys.h>

namespace FE::Input::Core
{
    struct KeyboardState final
    {
        festd::bitset<256> m_keyDownMap;
        festd::bitset<256> m_keyUpMap;
        festd::bitset<256> m_keyPressMap;
    };


    struct MouseState final
    {
        festd::bitset<5> m_buttonDownMap;
        festd::bitset<5> m_buttonUpMap;
        festd::bitset<5> m_buttonPressMap;
        Vector2 m_delta{ 0, 0 };
        float m_wheelDelta = 0;
    };


    //! Ordered device snapshot. Time is the interval spent in the preceding held state.
    //! Motion and wheel values apply only to this sample.
    struct Sample final
    {
        KeyboardState m_keyboard;
        MouseState m_mouse;
        float m_deltaSeconds = 0;
    };


    enum class DeviceKind : uint32_t
    {
        kKeyboard,
        kMouse,
    };


    struct Backend
    {
        virtual ~Backend() = default;

        [[nodiscard]] virtual festd::span<const DeviceKind> EnumerateDevices() const = 0;

        //! Poll self-driven devices. Platform-fed backends receive events from their application instead.
        virtual void Update() {}

        //! Nonzero sequence identifies an ordered snapshot; zero means an unsampled state backend.
        [[nodiscard]] virtual uint64_t GetSampleSequence() const
        {
            return 0;
        }

        [[nodiscard]] virtual festd::span<const Sample> GetSamples() const
        {
            return {};
        }

        [[nodiscard]] virtual const KeyboardState& GetKeyboardState() const = 0;
        [[nodiscard]] virtual const MouseState& GetMouseState() const = 0;
    };
} // namespace FE::Input::Core
