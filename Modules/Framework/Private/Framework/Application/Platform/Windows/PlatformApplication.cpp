#include <FeCore/DI/Activator.h>
#include <FeCore/Memory/FiberTempAllocator.h>
#include <Framework/Application/Core/PlatformEvent.h>
#include <Framework/Application/Core/PlatformMonitor.h>
#include <Framework/Application/Platform/Windows/PlatformApplication.h>
#include <Framework/Application/Platform/Windows/PlatformWindow.h>
#include <shellscalingapi.h>
#include <windowsx.h>

#pragma comment(lib, "Shcore.lib")

namespace FE::Framework::Windows
{
    namespace
    {
        RectInt ConvertRect(const RECT rect)
        {
            return festd::bit_cast<RectInt>(rect);
        }


        float GetMonitorDPI(const HMONITOR monitor)
        {
            UINT xDpi, yDpi;
            const HRESULT hr = GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &xDpi, &yDpi);
            FE_Assert(SUCCEEDED(hr));
            return static_cast<float>(xDpi) / 96.0f;
        }


        UINT ConvertKind(const Core::AlertKind kind) noexcept
        {
            switch (kind)
            {
            case Core::AlertKind::kInformation:
                return MB_ICONINFORMATION;
            case Core::AlertKind::kWarning:
                return MB_ICONWARNING;
            case Core::AlertKind::kError:
                return MB_ICONERROR;
            case Core::AlertKind::kQuestion:
                return MB_ICONQUESTION;
            default:
                return MB_ICONINFORMATION;
            }
        }


        UINT ConvertButtons(const Core::AlertButtons buttons) noexcept
        {
            switch (buttons)
            {
            case Core::AlertButtons::kOk:
                return MB_OK;
            case Core::AlertButtons::kOkCancel:
                return MB_OKCANCEL;
            case Core::AlertButtons::kYesNo:
                return MB_YESNO;
            default:
                return MB_OK;
            }
        }


        const char* WindowMessageToString(const UINT message)
        {
            switch (message)
            {
                // clang-format off
            case WM_ACTIVATE: return "WM_ACTIVATE";
            case WM_ACTIVATEAPP: return "WM_ACTIVATEAPP";
            case WM_AFXFIRST: return "WM_AFXFIRST";
            case WM_AFXLAST: return "WM_AFXLAST";
            case WM_APP: return "WM_APP";
            case WM_APPCOMMAND: return "WM_APPCOMMAND";
            case WM_ASKCBFORMATNAME: return "WM_ASKCBFORMATNAME";
            case WM_CANCELJOURNAL: return "WM_CANCELJOURNAL";
            case WM_CANCELMODE: return "WM_CANCELMODE";
            case WM_CAPTURECHANGED: return "WM_CAPTURECHANGED";
            case WM_CHANGECBCHAIN: return "WM_CHANGECBCHAIN";
            case WM_CHANGEUISTATE: return "WM_CHANGEUISTATE";
            case WM_CHAR: return "WM_CHAR";
            case WM_CHARTOITEM: return "WM_CHARTOITEM";
            case WM_CHILDACTIVATE: return "WM_CHILDACTIVATE";
            case WM_CLEAR: return "WM_CLEAR";
            case WM_CLOSE: return "WM_CLOSE";
            case WM_COMMAND: return "WM_COMMAND";
            case WM_COMMNOTIFY: return "WM_COMMNOTIFY";
            case WM_COMPACTING: return "WM_COMPACTING";
            case WM_COMPAREITEM: return "WM_COMPAREITEM";
            case WM_CONTEXTMENU: return "WM_CONTEXTMENU";
            case WM_COPY: return "WM_COPY";
            case WM_COPYDATA: return "WM_COPYDATA";
            case WM_CREATE: return "WM_CREATE";
            case WM_CTLCOLORBTN: return "WM_CTLCOLORBTN";
            case WM_CTLCOLORDLG: return "WM_CTLCOLORDLG";
            case WM_CTLCOLOREDIT: return "WM_CTLCOLOREDIT";
            case WM_CTLCOLORLISTBOX: return "WM_CTLCOLORLISTBOX";
            case WM_CTLCOLORMSGBOX: return "WM_CTLCOLORMSGBOX";
            case WM_CTLCOLORSCROLLBAR: return "WM_CTLCOLORSCROLLBAR";
            case WM_CTLCOLORSTATIC: return "WM_CTLCOLORSTATIC";
            case WM_CUT: return "WM_CUT";
            case WM_DEADCHAR: return "WM_DEADCHAR";
            case WM_DELETEITEM: return "WM_DELETEITEM";
            case WM_DESTROY: return "WM_DESTROY";
            case WM_DESTROYCLIPBOARD: return "WM_DESTROYCLIPBOARD";
            case WM_DEVICECHANGE: return "WM_DEVICECHANGE";
            case WM_DEVMODECHANGE: return "WM_DEVMODECHANGE";
            case WM_DISPLAYCHANGE: return "WM_DISPLAYCHANGE";
            case WM_DRAWCLIPBOARD: return "WM_DRAWCLIPBOARD";
            case WM_DRAWITEM: return "WM_DRAWITEM";
            case WM_DROPFILES: return "WM_DROPFILES";
            case WM_ENABLE: return "WM_ENABLE";
            case WM_ENDSESSION: return "WM_ENDSESSION";
            case WM_ENTERIDLE: return "WM_ENTERIDLE";
            case WM_ENTERMENULOOP: return "WM_ENTERMENULOOP";
            case WM_ENTERSIZEMOVE: return "WM_ENTERSIZEMOVE";
            case WM_ERASEBKGND: return "WM_ERASEBKGND";
            case WM_EXITMENULOOP: return "WM_EXITMENULOOP";
            case WM_EXITSIZEMOVE: return "WM_EXITSIZEMOVE";
            case WM_FONTCHANGE: return "WM_FONTCHANGE";
            case WM_GETDLGCODE: return "WM_GETDLGCODE";
            case WM_GETFONT: return "WM_GETFONT";
            case WM_GETHOTKEY: return "WM_GETHOTKEY";
            case WM_GETICON: return "WM_GETICON";
            case WM_GETMINMAXINFO: return "WM_GETMINMAXINFO";
            case WM_GETOBJECT: return "WM_GETOBJECT";
            case WM_GETTEXT: return "WM_GETTEXT";
            case WM_GETTEXTLENGTH: return "WM_GETTEXTLENGTH";
            case WM_HANDHELDFIRST: return "WM_HANDHELDFIRST";
            case WM_HANDHELDLAST: return "WM_HANDHELDLAST";
            case WM_HELP: return "WM_HELP";
            case WM_HOTKEY: return "WM_HOTKEY";
            case WM_HSCROLL: return "WM_HSCROLL";
            case WM_HSCROLLCLIPBOARD: return "WM_HSCROLLCLIPBOARD";
            case WM_ICONERASEBKGND: return "WM_ICONERASEBKGND";
            case WM_IME_CHAR: return "WM_IME_CHAR";
            case WM_IME_COMPOSITION: return "WM_IME_COMPOSITION";
            case WM_IME_COMPOSITIONFULL: return "WM_IME_COMPOSITIONFULL";
            case WM_IME_CONTROL: return "WM_IME_CONTROL";
            case WM_IME_ENDCOMPOSITION: return "WM_IME_ENDCOMPOSITION";
            case WM_IME_KEYDOWN: return "WM_IME_KEYDOWN";
            case WM_IME_KEYUP: return "WM_IME_KEYUP";
            case WM_IME_NOTIFY: return "WM_IME_NOTIFY";
            case WM_IME_REQUEST: return "WM_IME_REQUEST";
            case WM_IME_SELECT: return "WM_IME_SELECT";
            case WM_IME_SETCONTEXT: return "WM_IME_SETCONTEXT";
            case WM_IME_STARTCOMPOSITION: return "WM_IME_STARTCOMPOSITION";
            case WM_INITDIALOG: return "WM_INITDIALOG";
            case WM_INITMENU: return "WM_INITMENU";
            case WM_INITMENUPOPUP: return "WM_INITMENUPOPUP";
            case WM_INPUT: return "WM_INPUT";
            case WM_INPUTLANGCHANGE: return "WM_INPUTLANGCHANGE";
            case WM_INPUTLANGCHANGEREQUEST: return "WM_INPUTLANGCHANGEREQUEST";
            case WM_KEYDOWN: return "WM_KEYDOWN";
            case WM_KEYLAST: return "WM_KEYLAST";
            case WM_KEYUP: return "WM_KEYUP";
            case WM_KILLFOCUS: return "WM_KILLFOCUS";
            case WM_LBUTTONDBLCLK: return "WM_LBUTTONDBLCLK";
            case WM_LBUTTONDOWN: return "WM_LBUTTONDOWN";
            case WM_LBUTTONUP: return "WM_LBUTTONUP";
            case WM_MBUTTONDBLCLK: return "WM_MBUTTONDBLCLK";
            case WM_MBUTTONDOWN: return "WM_MBUTTONDOWN";
            case WM_MBUTTONUP: return "WM_MBUTTONUP";
            case WM_MDIACTIVATE: return "WM_MDIACTIVATE";
            case WM_MDICASCADE: return "WM_MDICASCADE";
            case WM_MDICREATE: return "WM_MDICREATE";
            case WM_MDIDESTROY: return "WM_MDIDESTROY";
            case WM_MDIGETACTIVE: return "WM_MDIGETACTIVE";
            case WM_MDIICONARRANGE: return "WM_MDIICONARRANGE";
            case WM_MDIMAXIMIZE: return "WM_MDIMAXIMIZE";
            case WM_MDINEXT: return "WM_MDINEXT";
            case WM_MDIREFRESHMENU: return "WM_MDIREFRESHMENU";
            case WM_MDIRESTORE: return "WM_MDIRESTORE";
            case WM_MDISETMENU: return "WM_MDISETMENU";
            case WM_MDITILE: return "WM_MDITILE";
            case WM_MEASUREITEM: return "WM_MEASUREITEM";
            case WM_MENUCHAR: return "WM_MENUCHAR";
            case WM_MENUCOMMAND: return "WM_MENUCOMMAND";
            case WM_MENUDRAG: return "WM_MENUDRAG";
            case WM_MENUGETOBJECT: return "WM_MENUGETOBJECT";
            case WM_MENURBUTTONUP: return "WM_MENURBUTTONUP";
            case WM_MENUSELECT: return "WM_MENUSELECT";
            case WM_MOUSEACTIVATE: return "WM_MOUSEACTIVATE";
            case WM_MOUSEFIRST: return "WM_MOUSEFIRST";
            case WM_MOUSEHOVER: return "WM_MOUSEHOVER";
            case WM_MOUSELAST: return "WM_MOUSELAST";
            case WM_MOUSELEAVE: return "WM_MOUSELEAVE";
            case WM_MOUSEWHEEL: return "WM_MOUSEWHEEL";
            case WM_MOVE: return "WM_MOVE";
            case WM_MOVING: return "WM_MOVING";
            case WM_NCACTIVATE: return "WM_NCACTIVATE";
            case WM_NCCALCSIZE: return "WM_NCCALCSIZE";
            case WM_NCCREATE: return "WM_NCCREATE";
            case WM_NCDESTROY: return "WM_NCDESTROY";
            case WM_NCHITTEST: return "WM_NCHITTEST";
            case WM_NCLBUTTONDBLCLK: return "WM_NCLBUTTONDBLCLK";
            case WM_NCLBUTTONDOWN: return "WM_NCLBUTTONDOWN";
            case WM_NCLBUTTONUP: return "WM_NCLBUTTONUP";
            case WM_NCMBUTTONDBLCLK: return "WM_NCMBUTTONDBLCLK";
            case WM_NCMBUTTONDOWN: return "WM_NCMBUTTONDOWN";
            case WM_NCMBUTTONUP: return "WM_NCMBUTTONUP";
            case WM_NCMOUSEHOVER: return "WM_NCMOUSEHOVER";
            case WM_NCMOUSELEAVE: return "WM_NCMOUSELEAVE";
            case WM_NCMOUSEMOVE: return "WM_NCMOUSEMOVE";
            case WM_NCPAINT: return "WM_NCPAINT";
            case WM_NCRBUTTONDBLCLK: return "WM_NCRBUTTONDBLCLK";
            case WM_NCRBUTTONDOWN: return "WM_NCRBUTTONDOWN";
            case WM_NCRBUTTONUP: return "WM_NCRBUTTONUP";
            case WM_NCXBUTTONDBLCLK: return "WM_NCXBUTTONDBLCLK";
            case WM_NCXBUTTONDOWN: return "WM_NCXBUTTONDOWN";
            case WM_NCXBUTTONUP: return "WM_NCXBUTTONUP";
            case WM_NEXTDLGCTL: return "WM_NEXTDLGCTL";
            case WM_NEXTMENU: return "WM_NEXTMENU";
            case WM_NOTIFY: return "WM_NOTIFY";
            case WM_NOTIFYFORMAT: return "WM_NOTIFYFORMAT";
            case WM_NULL: return "WM_NULL";
            case WM_PAINT: return "WM_PAINT";
            case WM_PAINTCLIPBOARD: return "WM_PAINTCLIPBOARD";
            case WM_PAINTICON: return "WM_PAINTICON";
            case WM_PALETTECHANGED: return "WM_PALETTECHANGED";
            case WM_PALETTEISCHANGING: return "WM_PALETTEISCHANGING";
            case WM_PARENTNOTIFY: return "WM_PARENTNOTIFY";
            case WM_PASTE: return "WM_PASTE";
            case WM_PENWINFIRST: return "WM_PENWINFIRST";
            case WM_PENWINLAST: return "WM_PENWINLAST";
            case WM_POWER: return "WM_POWER";
            case WM_POWERBROADCAST: return "WM_POWERBROADCAST";
            case WM_PRINT: return "WM_PRINT";
            case WM_PRINTCLIENT: return "WM_PRINTCLIENT";
            case WM_QUERYDRAGICON: return "WM_QUERYDRAGICON";
            case WM_QUERYENDSESSION: return "WM_QUERYENDSESSION";
            case WM_QUERYNEWPALETTE: return "WM_QUERYNEWPALETTE";
            case WM_QUERYOPEN: return "WM_QUERYOPEN";
            case WM_QUERYUISTATE: return "WM_QUERYUISTATE";
            case WM_QUEUESYNC: return "WM_QUEUESYNC";
            case WM_QUIT: return "WM_QUIT";
            case WM_RBUTTONDBLCLK: return "WM_RBUTTONDBLCLK";
            case WM_RBUTTONDOWN: return "WM_RBUTTONDOWN";
            case WM_RBUTTONUP: return "WM_RBUTTONUP";
            case WM_RENDERALLFORMATS: return "WM_RENDERALLFORMATS";
            case WM_RENDERFORMAT: return "WM_RENDERFORMAT";
            case WM_SETCURSOR: return "WM_SETCURSOR";
            case WM_SETFOCUS: return "WM_SETFOCUS";
            case WM_SETFONT: return "WM_SETFONT";
            case WM_SETHOTKEY: return "WM_SETHOTKEY";
            case WM_SETICON: return "WM_SETICON";
            case WM_SETREDRAW: return "WM_SETREDRAW";
            case WM_SETTEXT: return "WM_SETTEXT";
            case WM_SETTINGCHANGE: return "WM_SETTINGCHANGE";
            case WM_SHOWWINDOW: return "WM_SHOWWINDOW";
            case WM_SIZE: return "WM_SIZE";
            case WM_SIZECLIPBOARD: return "WM_SIZECLIPBOARD";
            case WM_SIZING: return "WM_SIZING";
            case WM_SPOOLERSTATUS: return "WM_SPOOLERSTATUS";
            case WM_STYLECHANGED: return "WM_STYLECHANGED";
            case WM_STYLECHANGING: return "WM_STYLECHANGING";
            case WM_SYNCPAINT: return "WM_SYNCPAINT";
            case WM_SYSCHAR: return "WM_SYSCHAR";
            case WM_SYSCOLORCHANGE: return "WM_SYSCOLORCHANGE";
            case WM_SYSCOMMAND: return "WM_SYSCOMMAND";
            case WM_SYSDEADCHAR: return "WM_SYSDEADCHAR";
            case WM_SYSKEYDOWN: return "WM_SYSKEYDOWN";
            case WM_SYSKEYUP: return "WM_SYSKEYUP";
            case WM_TABLET_FIRST: return "WM_TABLET_FIRST";
            case WM_TABLET_LAST: return "WM_TABLET_LAST";
            case WM_TCARD: return "WM_TCARD";
            case WM_THEMECHANGED: return "WM_THEMECHANGED";
            case WM_TIMECHANGE: return "WM_TIMECHANGE";
            case WM_TIMER: return "WM_TIMER";
            case WM_UNDO: return "WM_UNDO";
            case WM_UNINITMENUPOPUP: return "WM_UNINITMENUPOPUP";
            case WM_UPDATEUISTATE: return "WM_UPDATEUISTATE";
            case WM_USER: return "WM_USER";
            case WM_USERCHANGED: return "WM_USERCHANGED";
            case WM_VKEYTOITEM: return "WM_VKEYTOITEM";
            case WM_VSCROLL: return "WM_VSCROLL";
            case WM_VSCROLLCLIPBOARD: return "WM_VSCROLLCLIPBOARD";
            case WM_WINDOWPOSCHANGED: return "WM_WINDOWPOSCHANGED";
            case WM_WINDOWPOSCHANGING: return "WM_WINDOWPOSCHANGING";
            case WM_WTSSESSION_CHANGE: return "WM_WTSSESSION_CHANGE";
            case WM_XBUTTONDBLCLK: return "WM_XBUTTONDBLCLK";
            case WM_XBUTTONDOWN: return "WM_XBUTTONDOWN";
            case WM_XBUTTONUP: return "WM_XBUTTONUP";
            default: return "Unknown";
                // clang-format on
            }
        }
    } // namespace


    LRESULT CALLBACK PlatformApplication::WindowProc(const HWND hWnd, const UINT message, WPARAM wParam, const LPARAM lParam)
    {
        FE_PROFILER_ZONE_TEXT("HWND: %p; MSG: %s, W: %ull, L: %ll", hWnd, WindowMessageToString(message), wParam, lParam);

        auto* window = reinterpret_cast<PlatformWindow*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
        if (window == nullptr)
            return DefWindowProcW(hWnd, message, wParam, lParam);

        auto* app = ImplCast(window->GetApplication());

        switch (message)
        {
        case WM_CLOSE:
            if (window == app->GetMainWindow())
                app->m_closeRequested = true;
            else
                app->m_events.push_back(Core::PlatformEvent::WindowClose(window));
            break;

        case WM_MOVE:
            app->m_events.push_back(Core::PlatformEvent::WindowMove(window));
            break;

        case WM_SIZE:
            app->m_events.push_back(Core::PlatformEvent::WindowResize(window));
            break;

        case WM_INPUTLANGCHANGE:
            app->UpdateKeyboardCodePage();
            break;

        case WM_DISPLAYCHANGE:
            app->UpdateMonitors();
            break;

        case WM_MOUSEMOVE:
        case WM_NCMOUSEMOVE:
            {
                const bool clientArea = message == WM_MOUSEMOVE;
                app->m_mouseTrackWindow = hWnd;

                const auto mouseTrackArea = clientArea ? MouseTrackArea::kClient : MouseTrackArea::kNonClient;
                if (app->m_mouseTrackArea != mouseTrackArea)
                {
                    TRACKMOUSEEVENT tmeCancel;
                    tmeCancel.cbSize = sizeof(tmeCancel);
                    tmeCancel.dwFlags = TME_CANCEL;
                    tmeCancel.hwndTrack = hWnd;
                    tmeCancel.dwHoverTime = 0;

                    TRACKMOUSEEVENT tmeTrack;
                    tmeTrack.cbSize = sizeof(tmeTrack);
                    tmeTrack.dwFlags = static_cast<DWORD>(clientArea ? TME_LEAVE : (TME_LEAVE | TME_NONCLIENT));
                    tmeTrack.hwndTrack = hWnd;
                    tmeTrack.dwHoverTime = 0;

                    if (app->m_mouseTrackArea != MouseTrackArea::kNone)
                        ::TrackMouseEvent(&tmeCancel);
                    ::TrackMouseEvent(&tmeTrack);

                    app->m_mouseTrackArea = mouseTrackArea;
                }

                POINT mousePosition{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                if (message == WM_MOUSEMOVE)
                    ClientToScreen(hWnd, &mousePosition);

                app->m_events.push_back(Core::PlatformEvent::MouseMove(mousePosition.x, mousePosition.y));
            }
            break;

        case WM_MOUSELEAVE:
        case WM_NCMOUSELEAVE:
            {
                const bool clientArea = message == WM_MOUSEMOVE;
                const auto mouseTrackArea = clientArea ? MouseTrackArea::kClient : MouseTrackArea::kNonClient;
                if (app->m_mouseTrackArea == mouseTrackArea)
                {
                    if (app->m_mouseTrackWindow == hWnd)
                        app->m_mouseTrackWindow = nullptr;
                    app->m_mouseTrackArea = MouseTrackArea::kNone;
                    app->m_events.push_back(Core::PlatformEvent::MouseLeave());
                }
            }
            break;

        case WM_MOUSEWHEEL:
            app->m_events.push_back(
                Core::PlatformEvent::MouseWheel(GET_WHEEL_DELTA_WPARAM(wParam) / static_cast<float>(WHEEL_DELTA)));
            break;

        case WM_DESTROY:
            if (app->m_mouseTrackWindow == hWnd && app->m_mouseTrackArea != MouseTrackArea::kNone)
            {
                TRACKMOUSEEVENT tmeCancel;
                tmeCancel.cbSize = sizeof(tmeCancel);
                tmeCancel.dwFlags = TME_CANCEL;
                tmeCancel.hwndTrack = hWnd;
                tmeCancel.dwHoverTime = 0;
                ::TrackMouseEvent(&tmeCancel);

                app->m_mouseTrackWindow = nullptr;
                app->m_mouseTrackArea = MouseTrackArea::kNone;
                app->m_events.push_back(Core::PlatformEvent::MouseLeave());
            }
            break;

        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONDBLCLK:
        case WM_XBUTTONDOWN:
        case WM_XBUTTONDBLCLK:
            {
                int32_t button = 0;
                if (message == WM_LBUTTONDOWN || message == WM_LBUTTONDBLCLK)
                    button = 0;
                if (message == WM_RBUTTONDOWN || message == WM_RBUTTONDBLCLK)
                    button = 1;
                if (message == WM_MBUTTONDOWN || message == WM_MBUTTONDBLCLK)
                    button = 2;
                if (message == WM_XBUTTONDOWN || message == WM_XBUTTONDBLCLK)
                    button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? 3 : 4;

                if (app->m_mouseButtonsDown == 0 && ::GetCapture() == nullptr)
                    ::SetCapture(hWnd);
                app->m_mouseButtonsDown |= 1 << button;
                app->m_events.push_back(Core::PlatformEvent::MouseDown(button));
            }
            break;

        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
        case WM_XBUTTONUP:
            {
                int32_t button = 0;
                if (message == WM_LBUTTONUP)
                    button = 0;
                if (message == WM_RBUTTONUP)
                    button = 1;
                if (message == WM_MBUTTONUP)
                    button = 2;
                if (message == WM_XBUTTONUP)
                    button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? 3 : 4;

                app->m_mouseButtonsDown &= ~(1 << button);
                if (app->m_mouseButtonsDown == 0 && ::GetCapture() == hWnd)
                    ::ReleaseCapture();

                app->m_events.push_back(Core::PlatformEvent::MouseUp(button));
            }
            break;

        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
            {
                const bool isDown = message == WM_KEYDOWN || message == WM_SYSKEYDOWN;
                const Input::Core::Key key = static_cast<Input::Core::Key>(wParam);

                app->m_events.push_back(Core::PlatformEvent::KeyboardKey(key, isDown));

                const auto isVkDown = [](const int32_t vk) {
                    return (::GetKeyState(vk) & 0x8000) != 0;
                };

                // AFAIR Windows does not send key down events for the print screen key...
                if (key == Input::Core::Key::kPrintScreen && !isDown)
                    app->m_events.push_back(Core::PlatformEvent::KeyboardKeyDown(key));

                if (wParam == VK_SHIFT)
                {
                    if (isVkDown(VK_LSHIFT) == isDown)
                        app->m_events.push_back(Core::PlatformEvent::KeyboardKey(Input::Core::Key::kLeftShift, isDown));
                    if (isVkDown(VK_RSHIFT) == isDown)
                        app->m_events.push_back(Core::PlatformEvent::KeyboardKey(Input::Core::Key::kRightShift, isDown));
                }
                else if (wParam == VK_CONTROL)
                {
                    if (isVkDown(VK_LCONTROL) == isDown)
                        app->m_events.push_back(Core::PlatformEvent::KeyboardKey(Input::Core::Key::kLeftCtrl, isDown));
                    if (isVkDown(VK_RCONTROL) == isDown)
                        app->m_events.push_back(Core::PlatformEvent::KeyboardKey(Input::Core::Key::kRightCtrl, isDown));
                }
                else if (wParam == VK_MENU)
                {
                    if (isVkDown(VK_LMENU) == isDown)
                        app->m_events.push_back(Core::PlatformEvent::KeyboardKey(Input::Core::Key::kLeftAlt, isDown));
                    if (isVkDown(VK_RMENU) == isDown)
                        app->m_events.push_back(Core::PlatformEvent::KeyboardKey(Input::Core::Key::kRightAlt, isDown));
                }
            }
            break;

        case WM_SETFOCUS:
            app->m_events.push_back(Core::PlatformEvent::WindowFocusChanged(window, true));
            break;

        case WM_KILLFOCUS:
            app->m_events.push_back(Core::PlatformEvent::WindowFocusChanged(window, false));
            break;

        case WM_CHAR:
            if (::IsWindowUnicode(hWnd))
            {
                app->m_events.push_back(Core::PlatformEvent::KeyboardCharInput(static_cast<wchar_t>(wParam)));
            }
            else
            {
                wchar_t wideChar = 0;
                ::MultiByteToWideChar(app->m_keyboardCodePage, MB_PRECOMPOSED, reinterpret_cast<char*>(&wParam), 1, &wideChar, 1);
                app->m_events.push_back(Core::PlatformEvent::KeyboardCharInput(wideChar));
            }
            break;
        default:
            break;
        }

        return DefWindowProcW(hWnd, message, wParam, lParam);
    }


    PlatformApplication::PlatformApplication()
    {
        FE_PROFILER_ZONE();

        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = &WindowProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.hIcon = nullptr;
        wc.hCursor = nullptr;
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BACKGROUND + 1);
        wc.lpszMenuName = nullptr;
        wc.lpszClassName = kWindowClassName;
        RegisterClassExW(&wc);

        SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

        UpdateKeyboardCodePage();
        UpdateMonitors();
    }


    PlatformApplication::~PlatformApplication()
    {
        UnregisterClassW(kWindowClassName, GetModuleHandleW(nullptr));
    }


    Core::AlertResponse PlatformApplication::ShowAlert(const Core::AlertKind kind, const festd::string_view title,
                                                       const festd::string_view message, const Core::AlertButtons buttons)
    {
        Memory::FiberTempAllocator tempAllocator;

        const Str::Utf8ToUtf16 wideTitle{ title.data(), title.size(), &tempAllocator };
        const Str::Utf8ToUtf16 wideMessage{ message.data(), message.size(), &tempAllocator };

        const UINT type = ConvertKind(kind) | ConvertButtons(buttons);
        const int32_t response = MessageBoxW(nullptr, wideMessage.ToWideString(), wideTitle.ToWideString(), type);
        switch (response)
        {
        case IDOK:
            return Core::AlertResponse::kOk;
        case IDCANCEL:
            return Core::AlertResponse::kCancel;
        case IDYES:
            return Core::AlertResponse::kYes;
        case IDNO:
            return Core::AlertResponse::kNo;
        default:
            return Core::AlertResponse::kNone;
        }
    }


    Core::PlatformWindow* PlatformApplication::CreateWindow(const Core::PlatformWindowDesc& desc)
    {
        PlatformWindow* window = DI::New<PlatformWindow>(m_windowPool.GetAllocator()).value();
        window->Init(desc);
        m_windows.push_back(window);
        return window;
    }


    void PlatformApplication::PollEvents()
    {
        FE_PROFILER_ZONE();

        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }


    bool PlatformApplication::IsCloseRequested() const
    {
        return m_closeRequested;
    }


    festd::span<const Core::PlatformEvent> PlatformApplication::GetEvents() const
    {
        return m_events;
    }


    festd::span<const Core::PlatformMonitorInfo> PlatformApplication::GetMonitors() const
    {
        return m_monitors;
    }


    festd::span<Core::PlatformWindow* const> PlatformApplication::GetWindows() const
    {
        return m_windows;
    }


    Core::PlatformWindow* PlatformApplication::GetMainWindow() const
    {
        if (m_windows.empty())
            return nullptr;

        return m_windows[0];
    }


    void PlatformApplication::UpdateKeyboardCodePage()
    {
        FE_PROFILER_ZONE();

        const HKL keyboardLayout = ::GetKeyboardLayout(0);
        const LCID keyboardLCID = MAKELCID(HIWORD(keyboardLayout), SORT_DEFAULT);
        const int32_t result = ::GetLocaleInfoA(keyboardLCID,
                                                LOCALE_RETURN_NUMBER | LOCALE_IDEFAULTANSICODEPAGE,
                                                reinterpret_cast<LPSTR>(&m_keyboardCodePage),
                                                sizeof(m_keyboardCodePage));

        if (result == 0)
            m_keyboardCodePage = CP_ACP;
    }


    void PlatformApplication::UpdateMonitors()
    {
        FE_PROFILER_ZONE();

        m_monitors.clear();

        const auto monitorEnumCallback = [](HMONITOR monitor, HDC, LPRECT, LPARAM data) -> BOOL {
            MONITORINFO nativeInfo;
            nativeInfo.cbSize = sizeof(MONITORINFO);
            if (!GetMonitorInfoW(monitor, &nativeInfo))
                return TRUE;

            Core::PlatformMonitorInfo info;
            info.m_rect = ConvertRect(nativeInfo.rcMonitor);
            info.m_workRect = ConvertRect(nativeInfo.rcWork);
            info.m_dpiScale = GetMonitorDPI(monitor);
            info.m_nativeHandle = Core::NativeMonitorHandle{ reinterpret_cast<uint64_t>(monitor) };

            if (info.m_dpiScale <= 0.0f)
                return TRUE;

            auto* app = reinterpret_cast<PlatformApplication*>(data);
            if (nativeInfo.dwFlags & MONITORINFOF_PRIMARY)
                app->m_monitors.insert(app->m_monitors.begin(), info);
            else
                app->m_monitors.push_back(info);

            return TRUE;
        };

        EnumDisplayMonitors(nullptr, nullptr, monitorEnumCallback, reinterpret_cast<LPARAM>(this));
        m_events.push_back(Core::PlatformEvent::MonitorsChanged());
    }
} // namespace FE::Framework::Windows
