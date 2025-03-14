#pragma once
#include <FeCore/Base/BaseTypes.h>
#include <Framework/Input/Core/Keys.h>

namespace FE::Framework::Core
{
    struct PlatformWindow;

    enum class PlatformEventType : uint32_t
    {
        kNone,
        kMonitorsChanged,
        kWindowClose,
        kWindowMove,
        kWindowResize,
        kWindowFocus,
        kWindowUnfocus,
        kKeyboardKeyDown,
        kKeyboardKeyUp,
        kKeyboardCharInput,
        kMouseDown,
        kMouseUp,
        kMouseWheel,
        kMouseMove,
        kMouseLeave,
        kCount,
    };


    struct PlatformEvent final
    {
        struct DummyPayload final
        {
            uint64_t m_dummy;
        };

        struct WindowPayload final
        {
            // TODO: this is the only payload that is more than a DWORD in size.
            //       Maybe we can replace it with some kind of ID.

            PlatformWindow* m_window;
        };

        struct KeyboardKeyPayload final
        {
            Input::Core::Key m_key;
        };

        struct KeyboardCharPayload final
        {
            uint32_t m_codepoint;
        };

        struct MouseButtonPayload final
        {
            uint32_t m_button;
        };

        struct MouseWheelPayload final
        {
            float m_delta;
        };

        struct MouseMovePayload final
        {
            uint32_t m_x : 16;
            uint32_t m_y : 16;
        };

        PlatformEventType m_type = PlatformEventType::kNone;
        union
        {
            DummyPayload m_dummyPayload = {};
            WindowPayload m_windowPayload;
            KeyboardKeyPayload m_keyboardKeyPayload;
            KeyboardCharPayload m_keyboardCharPayload;
            MouseButtonPayload m_mouseButtonPayload;
            MouseWheelPayload m_mouseWheelPayload;
            MouseMovePayload m_mouseMovePayload;
        };

        static PlatformEvent MonitorsChanged()
        {
            return { PlatformEventType::kMonitorsChanged, {} };
        }

        static PlatformEvent WindowClose(PlatformWindow* window)
        {
            PlatformEvent event;
            event.m_type = PlatformEventType::kWindowClose;
            event.m_windowPayload.m_window = window;
            return event;
        }

        static PlatformEvent WindowMove(PlatformWindow* window)
        {
            PlatformEvent event;
            event.m_type = PlatformEventType::kWindowMove;
            event.m_windowPayload.m_window = window;
            return event;
        }

        static PlatformEvent WindowResize(PlatformWindow* window)
        {
            PlatformEvent event;
            event.m_type = PlatformEventType::kWindowResize;
            event.m_windowPayload.m_window = window;
            return event;
        }

        static PlatformEvent WindowFocusChanged(PlatformWindow* window, const bool focused)
        {
            PlatformEvent event;
            event.m_type = focused ? PlatformEventType::kWindowFocus : PlatformEventType::kWindowUnfocus;
            event.m_windowPayload.m_window = window;
            return event;
        }

        static PlatformEvent KeyboardKeyDown(const Input::Core::Key key)
        {
            PlatformEvent event;
            event.m_type = PlatformEventType::kKeyboardKeyDown;
            event.m_keyboardKeyPayload.m_key = key;
            return event;
        }

        static PlatformEvent KeyboardKeyUp(const Input::Core::Key key)
        {
            PlatformEvent event;
            event.m_type = PlatformEventType::kKeyboardKeyUp;
            event.m_keyboardKeyPayload.m_key = key;
            return event;
        }

        static PlatformEvent KeyboardKey(const Input::Core::Key key, const bool down)
        {
            return down ? KeyboardKeyDown(key) : KeyboardKeyUp(key);
        }

        static PlatformEvent KeyboardCharInput(const uint32_t codepoint)
        {
            PlatformEvent event;
            event.m_type = PlatformEventType::kKeyboardCharInput;
            event.m_keyboardCharPayload.m_codepoint = codepoint;
            return event;
        }

        static PlatformEvent MouseDown(const uint32_t button)
        {
            PlatformEvent event;
            event.m_type = PlatformEventType::kMouseDown;
            event.m_mouseButtonPayload.m_button = button;
            return event;
        }

        static PlatformEvent MouseUp(const uint32_t button)
        {
            PlatformEvent event;
            event.m_type = PlatformEventType::kMouseUp;
            event.m_mouseButtonPayload.m_button = button;
            return event;
        }

        static PlatformEvent MouseWheel(const float delta)
        {
            PlatformEvent event;
            event.m_type = PlatformEventType::kMouseWheel;
            event.m_mouseWheelPayload.m_delta = delta;
            return event;
        }

        static PlatformEvent MouseMove(const uint32_t x, const uint32_t y)
        {
            PlatformEvent event;
            event.m_type = PlatformEventType::kMouseMove;
            event.m_mouseMovePayload.m_x = x;
            event.m_mouseMovePayload.m_y = y;
            return event;
        }

        static PlatformEvent MouseLeave()
        {
            PlatformEvent event;
            event.m_type = PlatformEventType::kMouseLeave;
            return event;
        }
    };
} // namespace FE::Framework::Core
