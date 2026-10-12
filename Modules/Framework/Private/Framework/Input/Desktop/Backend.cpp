#include <Core/Time/BaseTime.h>
#include <Framework/Application/Core/PlatformApplication.h>
#include <Framework/Input/Desktop/Backend.h>

namespace FE::Input::Desktop
{
    festd::span<const Core::DeviceKind> Backend::EnumerateDevices() const
    {
        static constexpr Core::DeviceKind devices[] = { Core::DeviceKind::kKeyboard, Core::DeviceKind::kMouse };
        return devices;
    }


    void Backend::Update()
    {
        const double now = Platform::GetTicks() * Platform::GetSecondsPerTick();
        const float elapsed = m_previousUpdateSeconds == 0 ? 0 : static_cast<float>(now - m_previousUpdateSeconds);
        Update(Framework::Core::PlatformApplication::Get().GetEvents(), elapsed, now);
        m_previousUpdateSeconds = now;
    }


    void Backend::Update(const festd::span<const Framework::Core::PlatformEvent> events, const float deltaSeconds,
                         const double frameEndSeconds)
    {
        using Framework::Core::PlatformEventType;
        ++m_sampleSequence;
        m_keyboard.m_keyPressMap.reset();
        m_keyboard.m_keyUpMap.reset();
        m_mouse.m_buttonPressMap.reset();
        m_mouse.m_buttonUpMap.reset();
        m_mouse.m_delta = { 0, 0 };
        m_mouse.m_wheelDelta = 0;

        m_samples.clear();
        m_samples.push_back({ m_keyboard, m_mouse, 0 });
        Vector2 accumulatedDelta{ 0, 0 };
        float accumulatedWheel = 0;
        double previous = frameEndSeconds - deltaSeconds;
        const double end = frameEndSeconds;
        for (const auto& event : events)
        {
            const double time = event.m_timestampSeconds == 0 ? end : Math::Clamp(event.m_timestampSeconds, previous, end);
            m_samples.push_back({ m_keyboard, m_mouse, static_cast<float>(time - previous) });
            m_samples.back().m_mouse.m_delta = { 0, 0 };
            m_samples.back().m_mouse.m_wheelDelta = 0;
            previous = time;
            m_keyboard.m_keyPressMap.reset();
            m_keyboard.m_keyUpMap.reset();
            m_mouse.m_buttonPressMap.reset();
            m_mouse.m_buttonUpMap.reset();
            m_mouse.m_delta = { 0, 0 };
            m_mouse.m_wheelDelta = 0;
            switch (event.m_type)
            {
            case PlatformEventType::kKeyboardKeyDown:
            case PlatformEventType::kKeyboardKeyUp:
                {
                    const uint32_t key = static_cast<uint32_t>(event.m_keyboardKeyPayload.m_key);
                    if (key == 0 || key >= 256)
                        break;

                    const bool down = event.m_type == PlatformEventType::kKeyboardKeyDown;
                    if (down && !m_keyboard.m_keyDownMap.test(key))
                        m_keyboard.m_keyPressMap.set(key);

                    if (!down && m_keyboard.m_keyDownMap.test(key))
                        m_keyboard.m_keyUpMap.set(key);

                    m_keyboard.m_keyDownMap.set(key, down);
                }
                break;

            case PlatformEventType::kMouseDown:
            case PlatformEventType::kMouseUp:
                {
                    const uint32_t button = event.m_mouseButtonPayload.m_button;
                    if (button >= 5)
                        break;

                    const bool down = event.m_type == PlatformEventType::kMouseDown;
                    if (down && !m_mouse.m_buttonDownMap.test(button))
                        m_mouse.m_buttonPressMap.set(button);

                    if (!down && m_mouse.m_buttonDownMap.test(button))
                        m_mouse.m_buttonUpMap.set(button);

                    m_mouse.m_buttonDownMap.set(button, down);
                }
                break;

            case PlatformEventType::kMouseMove:
                {
                    const Vector2 position{ static_cast<float>(static_cast<int16_t>(event.m_mouseMovePayload.m_x)),
                                            static_cast<float>(static_cast<int16_t>(event.m_mouseMovePayload.m_y)) };
                    if (m_hasPosition)
                        m_mouse.m_delta = m_mouse.m_delta + position - m_position;

                    m_position = position;
                    m_hasPosition = true;
                }
                break;

            case PlatformEventType::kMouseWheel:
                m_mouse.m_wheelDelta += event.m_mouseWheelPayload.m_delta;
                break;

            case PlatformEventType::kWindowUnfocus:
                m_keyboard.m_keyUpMap |= m_keyboard.m_keyDownMap;
                m_keyboard.m_keyDownMap.reset();
                m_keyboard.m_keyPressMap.reset();
                m_mouse.m_buttonUpMap |= m_mouse.m_buttonDownMap;
                m_mouse.m_buttonDownMap.reset();
                m_mouse.m_buttonPressMap.reset();
                m_mouse.m_delta = { 0, 0 };
                m_mouse.m_wheelDelta = 0;
                m_hasPosition = false;
                break;

            case PlatformEventType::kMouseLeave:
            case PlatformEventType::kWindowFocus:
                m_hasPosition = false;
                break;

            default:
                break;
            }

            accumulatedDelta = accumulatedDelta + m_mouse.m_delta;
            accumulatedWheel += m_mouse.m_wheelDelta;
            m_samples.push_back({ m_keyboard, m_mouse, 0 });
        }

        m_keyboard.m_keyPressMap.reset();
        m_keyboard.m_keyUpMap.reset();
        m_mouse.m_buttonPressMap.reset();
        m_mouse.m_buttonUpMap.reset();
        m_mouse.m_delta = { 0, 0 };
        m_mouse.m_wheelDelta = 0;
        m_samples.push_back({ m_keyboard, m_mouse, static_cast<float>(end - previous) });
        for (const Core::Sample& sample : m_samples)
        {
            m_keyboard.m_keyPressMap |= sample.m_keyboard.m_keyPressMap;
            m_keyboard.m_keyUpMap |= sample.m_keyboard.m_keyUpMap;
            m_mouse.m_buttonPressMap |= sample.m_mouse.m_buttonPressMap;
            m_mouse.m_buttonUpMap |= sample.m_mouse.m_buttonUpMap;
        }

        m_mouse.m_delta = accumulatedDelta;
        m_mouse.m_wheelDelta = accumulatedWheel;
    }
} // namespace FE::Input::Desktop
