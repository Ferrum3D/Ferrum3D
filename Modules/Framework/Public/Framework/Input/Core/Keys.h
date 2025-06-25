#pragma once
#include <FeCore/Base/BaseTypes.h>

namespace FE::Input::Core
{
    // clang-format off
    //        Name                                        Windows virtual key
#define FE_EXPAND_KEYS(_Func)                                                                                                    \
        _Func(Tab,                                      (0x09)) /* VK_TAB */                                                     \
        _Func(LeftArrow,                                (0x25)) /* VK_LEFT */                                                    \
        _Func(RightArrow,                               (0x27)) /* VK_RIGHT */                                                   \
        _Func(UpArrow,                                  (0x26)) /* VK_UP */                                                      \
        _Func(DownArrow,                                (0x28)) /* VK_DOWN */                                                    \
        _Func(PageUp,                                   (0x21)) /* VK_PRIOR */                                                   \
        _Func(PageDown,                                 (0x22)) /* VK_NEXT */                                                    \
        _Func(Home,                                     (0x24)) /* VK_HOME */                                                    \
        _Func(End,                                      (0x23)) /* VK_END */                                                     \
        _Func(Insert,                                   (0x2D)) /* VK_INSERT */                                                  \
        _Func(Delete,                                   (0x2E)) /* VK_DELETE */                                                  \
        _Func(Backspace,                                (0x08)) /* VK_BACK */                                                    \
        _Func(Space,                                    (0x20)) /* VK_SPACE */                                                   \
        _Func(Enter,                                    (0x0D)) /* VK_RETURN */                                                  \
        _Func(Escape,                                   (0x1B)) /* VK_ESCAPE */                                                  \
        _Func(Apostrophe,                               (0xDE)) /* VK_OEM_7 */                                                   \
        _Func(Comma,                                    (0xBC)) /* VK_OEM_COMMA */                                               \
        _Func(Minus,                                    (0xBD)) /* VK_OEM_MINUS */                                               \
        _Func(Period,                                   (0xBE)) /* VK_OEM_PERIOD */                                              \
        _Func(Slash,                                    (0xBF)) /* VK_OEM_2 */                                                   \
        _Func(Semicolon,                                (0xBA)) /* VK_OEM_1 */                                                   \
        _Func(Equal,                                    (0xBB)) /* VK_OEM_PLUS */                                                \
        _Func(LeftBracket,                              (0xDB)) /* VK_OEM_4 */                                                   \
        _Func(Backslash,                                (0xDC)) /* VK_OEM_5 */                                                   \
        _Func(RightBracket,                             (0xDD)) /* VK_OEM_6 */                                                   \
        _Func(GraveAccent,                              (0xC0)) /* VK_OEM_3 */                                                   \
        _Func(CapsLock,                                 (0x14)) /* VK_CAPITAL */                                                 \
        _Func(ScrollLock,                               (0x91)) /* VK_SCROLL */                                                  \
        _Func(NumLock,                                  (0x90)) /* VK_NUMLOCK */                                                 \
        _Func(PrintScreen,                              (0x2C)) /* VK_SNAPSHOT */                                                \
        _Func(Pause,                                    (0x13)) /* VK_PAUSE */                                                   \
        _Func(Keypad0,                                  (0x60)) /* VK_NUMPAD0 */                                                 \
        _Func(Keypad1,                                  (0x61)) /* VK_NUMPAD1 */                                                 \
        _Func(Keypad2,                                  (0x62)) /* VK_NUMPAD2 */                                                 \
        _Func(Keypad3,                                  (0x63)) /* VK_NUMPAD3 */                                                 \
        _Func(Keypad4,                                  (0x64)) /* VK_NUMPAD4 */                                                 \
        _Func(Keypad5,                                  (0x65)) /* VK_NUMPAD5 */                                                 \
        _Func(Keypad6,                                  (0x66)) /* VK_NUMPAD6 */                                                 \
        _Func(Keypad7,                                  (0x67)) /* VK_NUMPAD7 */                                                 \
        _Func(Keypad8,                                  (0x68)) /* VK_NUMPAD8 */                                                 \
        _Func(Keypad9,                                  (0x69)) /* VK_NUMPAD9 */                                                 \
        _Func(KeypadDecimal,                            (0x6E)) /* VK_DECIMAL */                                                 \
        _Func(KeypadDivide,                             (0x6F)) /* VK_DIVIDE */                                                  \
        _Func(KeypadMultiply,                           (0x6A)) /* VK_MULTIPLY */                                                \
        _Func(KeypadSubtract,                           (0x6D)) /* VK_SUBTRACT */                                                \
        _Func(KeypadAdd,                                (0x6B)) /* VK_ADD */                                                     \
        _Func(LeftShift,                                (0xA0)) /* VK_LSHIFT */                                                  \
        _Func(LeftCtrl,                                 (0xA2)) /* VK_LCONTROL */                                                \
        _Func(LeftAlt,                                  (0xA4)) /* VK_LMENU */                                                   \
        _Func(LeftSuper,                                (0x5B)) /* VK_LWIN */                                                    \
        _Func(RightShift,                               (0xA1)) /* VK_RSHIFT */                                                  \
        _Func(RightCtrl,                                (0xA3)) /* VK_RCONTROL */                                                \
        _Func(RightAlt,                                 (0xA5)) /* VK_RMENU */                                                   \
        _Func(RightSuper,                               (0x5C)) /* VK_RWIN */                                                    \
        _Func(Menu,                                     (0x5D)) /* VK_APPS */                                                    \
        _Func(Keyboard0,            static_cast<uint32_t>('0')) /* '0' */                                                        \
        _Func(Keyboard1,            static_cast<uint32_t>('1')) /* '1' */                                                        \
        _Func(Keyboard2,            static_cast<uint32_t>('2')) /* '2' */                                                        \
        _Func(Keyboard3,            static_cast<uint32_t>('3')) /* '3' */                                                        \
        _Func(Keyboard4,            static_cast<uint32_t>('4')) /* '4' */                                                        \
        _Func(Keyboard5,            static_cast<uint32_t>('5')) /* '5' */                                                        \
        _Func(Keyboard6,            static_cast<uint32_t>('6')) /* '6' */                                                        \
        _Func(Keyboard7,            static_cast<uint32_t>('7')) /* '7' */                                                        \
        _Func(Keyboard8,            static_cast<uint32_t>('8')) /* '8' */                                                        \
        _Func(Keyboard9,            static_cast<uint32_t>('9')) /* '9' */                                                        \
        _Func(A,                    static_cast<uint32_t>('A')) /* 'A' */                                                        \
        _Func(B,                    static_cast<uint32_t>('B')) /* 'B' */                                                        \
        _Func(C,                    static_cast<uint32_t>('C')) /* 'C' */                                                        \
        _Func(D,                    static_cast<uint32_t>('D')) /* 'D' */                                                        \
        _Func(E,                    static_cast<uint32_t>('E')) /* 'E' */                                                        \
        _Func(F,                    static_cast<uint32_t>('F')) /* 'F' */                                                        \
        _Func(G,                    static_cast<uint32_t>('G')) /* 'G' */                                                        \
        _Func(H,                    static_cast<uint32_t>('H')) /* 'H' */                                                        \
        _Func(I,                    static_cast<uint32_t>('I')) /* 'I' */                                                        \
        _Func(J,                    static_cast<uint32_t>('J')) /* 'J' */                                                        \
        _Func(K,                    static_cast<uint32_t>('K')) /* 'K' */                                                        \
        _Func(L,                    static_cast<uint32_t>('L')) /* 'L' */                                                        \
        _Func(M,                    static_cast<uint32_t>('M')) /* 'M' */                                                        \
        _Func(N,                    static_cast<uint32_t>('N')) /* 'N' */                                                        \
        _Func(O,                    static_cast<uint32_t>('O')) /* 'O' */                                                        \
        _Func(P,                    static_cast<uint32_t>('P')) /* 'P' */                                                        \
        _Func(Q,                    static_cast<uint32_t>('Q')) /* 'Q' */                                                        \
        _Func(R,                    static_cast<uint32_t>('R')) /* 'R' */                                                        \
        _Func(S,                    static_cast<uint32_t>('S')) /* 'S' */                                                        \
        _Func(T,                    static_cast<uint32_t>('T')) /* 'T' */                                                        \
        _Func(U,                    static_cast<uint32_t>('U')) /* 'U' */                                                        \
        _Func(V,                    static_cast<uint32_t>('V')) /* 'V' */                                                        \
        _Func(W,                    static_cast<uint32_t>('W')) /* 'W' */                                                        \
        _Func(X,                    static_cast<uint32_t>('X')) /* 'X' */                                                        \
        _Func(Y,                    static_cast<uint32_t>('Y')) /* 'Y' */                                                        \
        _Func(Z,                    static_cast<uint32_t>('Z')) /* 'Z' */                                                        \
        _Func(F1,                                       (0x70)) /* VK_F1 */                                                      \
        _Func(F2,                                       (0x71)) /* VK_F2 */                                                      \
        _Func(F3,                                       (0x72)) /* VK_F3 */                                                      \
        _Func(F4,                                       (0x73)) /* VK_F4 */                                                      \
        _Func(F5,                                       (0x74)) /* VK_F5 */                                                      \
        _Func(F6,                                       (0x75)) /* VK_F6 */                                                      \
        _Func(F7,                                       (0x76)) /* VK_F7 */                                                      \
        _Func(F8,                                       (0x77)) /* VK_F8 */                                                      \
        _Func(F9,                                       (0x78)) /* VK_F9 */                                                      \
        _Func(F10,                                      (0x79)) /* VK_F10 */                                                     \
        _Func(F11,                                      (0x7A)) /* VK_F11 */                                                     \
        _Func(F12,                                      (0x7B)) /* VK_F12 */                                                     \
        _Func(F13,                                      (0x7C)) /* VK_F13 */                                                     \
        _Func(F14,                                      (0x7D)) /* VK_F14 */                                                     \
        _Func(F15,                                      (0x7E)) /* VK_F15 */                                                     \
        _Func(F16,                                      (0x7F)) /* VK_F16 */                                                     \
        _Func(F17,                                      (0x80)) /* VK_F17 */                                                     \
        _Func(F18,                                      (0x81)) /* VK_F18 */                                                     \
        _Func(F19,                                      (0x82)) /* VK_F19 */                                                     \
        _Func(F20,                                      (0x83)) /* VK_F20 */                                                     \
        _Func(F21,                                      (0x84)) /* VK_F21 */                                                     \
        _Func(F22,                                      (0x85)) /* VK_F22 */                                                     \
        _Func(F23,                                      (0x86)) /* VK_F23 */                                                     \
        _Func(F24,                                      (0x87)) /* VK_F24 */                                                     \
        _Func(AppBack,                                  (0xA6)) /* VK_BROWSER_BACK */                                            \
        _Func(AppForward,                               (0xA7)) /* VK_BROWSER_FORWARD */
    // clang-format on


    enum class Key : uint32_t
    {
        kInvalid = 0,

#define FE_DECLARE_KEY(name, vk) k##name = (vk),
        FE_EXPAND_KEYS(FE_DECLARE_KEY)
#undef FE_DECLARE_KEY
    };
} // namespace FE::Input::Core
