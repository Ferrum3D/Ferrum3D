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


    enum class DeviceKind : uint32_t
    {
        kKeyboard,
        kMouse,
    };


    struct Backend
    {
        virtual ~Backend() = default;
        [[nodiscard]] virtual festd::span<const DeviceKind> EnumerateDevices() const = 0;
        virtual void Update() = 0;
        [[nodiscard]] virtual const KeyboardState& GetKeyboardState() const = 0;
        [[nodiscard]] virtual const MouseState& GetMouseState() const = 0;
    };
} // namespace FE::Input::Core
