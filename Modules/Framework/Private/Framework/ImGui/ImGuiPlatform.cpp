#include <Core/Time/BaseTime.h>
#include <Framework/Application/Core/PlatformEvent.h>
#include <Framework/Application/Core/PlatformMonitor.h>
#include <Framework/ImGui/ImGuiPlatform.h>
#include <imgui.h>

namespace FE::Framework
{
    namespace
    {
        using Input::Core::Key;

        constexpr ImGuiBackendFlags kPlatformBackendFlags =
            ImGuiBackendFlags_HasMouseCursors | ImGuiBackendFlags_PlatformHasViewports;


        void* AllocateImGuiMemory(const size_t byteSize, void*)
        {
            return Memory::DefaultAllocate(byteSize);
        }


        void FreeImGuiMemory(void* ptr, void*)
        {
            if (ptr != nullptr)
                Memory::DefaultFree(ptr);
        }


        ImGuiKey ConvertKey(const Key key)
        {
            switch (key)
            {
                // clang-format off
            case Key::kTab: return ImGuiKey_Tab;
            case Key::kLeftArrow: return ImGuiKey_LeftArrow;
            case Key::kRightArrow: return ImGuiKey_RightArrow;
            case Key::kUpArrow: return ImGuiKey_UpArrow;
            case Key::kDownArrow: return ImGuiKey_DownArrow;
            case Key::kPageUp: return ImGuiKey_PageUp;
            case Key::kPageDown: return ImGuiKey_PageDown;
            case Key::kHome: return ImGuiKey_Home;
            case Key::kEnd: return ImGuiKey_End;
            case Key::kInsert: return ImGuiKey_Insert;
            case Key::kDelete: return ImGuiKey_Delete;
            case Key::kBackspace: return ImGuiKey_Backspace;
            case Key::kSpace: return ImGuiKey_Space;
            case Key::kEnter: return ImGuiKey_Enter;
            case Key::kEscape: return ImGuiKey_Escape;
            case Key::kApostrophe: return ImGuiKey_Apostrophe;
            case Key::kComma: return ImGuiKey_Comma;
            case Key::kMinus: return ImGuiKey_Minus;
            case Key::kPeriod: return ImGuiKey_Period;
            case Key::kSlash: return ImGuiKey_Slash;
            case Key::kSemicolon: return ImGuiKey_Semicolon;
            case Key::kEqual: return ImGuiKey_Equal;
            case Key::kLeftBracket: return ImGuiKey_LeftBracket;
            case Key::kBackslash: return ImGuiKey_Backslash;
            case Key::kRightBracket: return ImGuiKey_RightBracket;
            case Key::kGraveAccent: return ImGuiKey_GraveAccent;
            case Key::kCapsLock: return ImGuiKey_CapsLock;
            case Key::kScrollLock: return ImGuiKey_ScrollLock;
            case Key::kNumLock: return ImGuiKey_NumLock;
            case Key::kPrintScreen: return ImGuiKey_PrintScreen;
            case Key::kPause: return ImGuiKey_Pause;
            case Key::kKeypad0: return ImGuiKey_Keypad0;
            case Key::kKeypad1: return ImGuiKey_Keypad1;
            case Key::kKeypad2: return ImGuiKey_Keypad2;
            case Key::kKeypad3: return ImGuiKey_Keypad3;
            case Key::kKeypad4: return ImGuiKey_Keypad4;
            case Key::kKeypad5: return ImGuiKey_Keypad5;
            case Key::kKeypad6: return ImGuiKey_Keypad6;
            case Key::kKeypad7: return ImGuiKey_Keypad7;
            case Key::kKeypad8: return ImGuiKey_Keypad8;
            case Key::kKeypad9: return ImGuiKey_Keypad9;
            case Key::kKeypadDecimal: return ImGuiKey_KeypadDecimal;
            case Key::kKeypadDivide: return ImGuiKey_KeypadDivide;
            case Key::kKeypadMultiply: return ImGuiKey_KeypadMultiply;
            case Key::kKeypadSubtract: return ImGuiKey_KeypadSubtract;
            case Key::kKeypadAdd: return ImGuiKey_KeypadAdd;
            case Key::kLeftShift: return ImGuiKey_LeftShift;
            case Key::kLeftCtrl: return ImGuiKey_LeftCtrl;
            case Key::kLeftAlt: return ImGuiKey_LeftAlt;
            case Key::kLeftSuper: return ImGuiKey_LeftSuper;
            case Key::kRightShift: return ImGuiKey_RightShift;
            case Key::kRightCtrl: return ImGuiKey_RightCtrl;
            case Key::kRightAlt: return ImGuiKey_RightAlt;
            case Key::kRightSuper: return ImGuiKey_RightSuper;
            case Key::kMenu: return ImGuiKey_Menu;
            case Key::kAppBack: return ImGuiKey_AppBack;
            case Key::kAppForward: return ImGuiKey_AppForward;
                // clang-format on
            default:
                break;
            }

            const uint32_t code = festd::to_underlying(key);
            if (code >= festd::to_underlying(Key::kKeyboard0) && code <= festd::to_underlying(Key::kKeyboard9))
                return static_cast<ImGuiKey>(ImGuiKey_0 + (code - festd::to_underlying(Key::kKeyboard0)));

            if (code >= festd::to_underlying(Key::kA) && code <= festd::to_underlying(Key::kZ))
                return static_cast<ImGuiKey>(ImGuiKey_A + (code - festd::to_underlying(Key::kA)));

            if (code >= festd::to_underlying(Key::kF1) && code <= festd::to_underlying(Key::kF24))
                return static_cast<ImGuiKey>(ImGuiKey_F1 + (code - festd::to_underlying(Key::kF1)));

            return ImGuiKey_None;
        }


        ImGuiKey GetModifier(const ImGuiKey key)
        {
            switch (key)
            {
            case ImGuiKey_LeftCtrl:
            case ImGuiKey_RightCtrl:
                return ImGuiMod_Ctrl;
            case ImGuiKey_LeftShift:
            case ImGuiKey_RightShift:
                return ImGuiMod_Shift;
            case ImGuiKey_LeftAlt:
            case ImGuiKey_RightAlt:
                return ImGuiMod_Alt;
            case ImGuiKey_LeftSuper:
            case ImGuiKey_RightSuper:
                return ImGuiMod_Super;
            default:
                return ImGuiKey_None;
            }
        }


        ImGuiKey GetOtherModifierSide(const ImGuiKey key)
        {
            switch (key)
            {
                // clang-format off
            case ImGuiKey_LeftCtrl: return ImGuiKey_RightCtrl;
            case ImGuiKey_RightCtrl: return ImGuiKey_LeftCtrl;
            case ImGuiKey_LeftShift: return ImGuiKey_RightShift;
            case ImGuiKey_RightShift: return ImGuiKey_LeftShift;
            case ImGuiKey_LeftAlt: return ImGuiKey_RightAlt;
            case ImGuiKey_RightAlt: return ImGuiKey_LeftAlt;
            case ImGuiKey_LeftSuper: return ImGuiKey_RightSuper;
            case ImGuiKey_RightSuper: return ImGuiKey_LeftSuper;
                // clang-format on
            default:
                return ImGuiKey_None;
            }
        }


        Core::PlatformCursor ConvertCursor(const ImGuiMouseCursor cursor)
        {
            switch (cursor)
            {
            case ImGuiMouseCursor_Arrow:
                return Core::PlatformCursor::kArrow;
            case ImGuiMouseCursor_TextInput:
                return Core::PlatformCursor::kTextInput;
            case ImGuiMouseCursor_ResizeAll:
                return Core::PlatformCursor::kResizeAll;
            case ImGuiMouseCursor_ResizeNS:
                return Core::PlatformCursor::kResizeNS;
            case ImGuiMouseCursor_ResizeEW:
                return Core::PlatformCursor::kResizeEW;
            case ImGuiMouseCursor_ResizeNESW:
                return Core::PlatformCursor::kResizeNESW;
            case ImGuiMouseCursor_ResizeNWSE:
                return Core::PlatformCursor::kResizeNWSE;
            case ImGuiMouseCursor_Hand:
                return Core::PlatformCursor::kHand;
            case ImGuiMouseCursor_Wait:
                return Core::PlatformCursor::kWait;
            case ImGuiMouseCursor_Progress:
                return Core::PlatformCursor::kProgress;
            case ImGuiMouseCursor_NotAllowed:
                return Core::PlatformCursor::kNotAllowed;
            default:
                return Core::PlatformCursor::kHidden;
            }
        }


        Core::PlatformWindowStyleFlags ConvertWindowStyle(const ImGuiViewportFlags flags)
        {
            Core::PlatformWindowStyleFlags style = Core::PlatformWindowStyleFlags::kNone;
            if (flags & ImGuiViewportFlags_NoDecoration)
                style |= Core::PlatformWindowStyleFlags::kNoDecoration;
            if (flags & ImGuiViewportFlags_NoTaskBarIcon)
                style |= Core::PlatformWindowStyleFlags::kNoTaskBarIcon;
            if (flags & ImGuiViewportFlags_TopMost)
                style |= Core::PlatformWindowStyleFlags::kTopMost;

            return style;
        }


        ImVec2 ToImVec2(const Vector2Int value)
        {
            return ImVec2(static_cast<float>(value.x), static_cast<float>(value.y));
        }


        Vector2Int ToVector2Int(const ImVec2 value)
        {
            return Vector2Int(static_cast<int32_t>(value.x), static_cast<int32_t>(value.y));
        }
    } // namespace


    //! Secondary viewports own their platform windows; the main viewport borrows the application window.
    struct ImGuiPlatform::ViewportData final
    {
        Rc<Core::PlatformWindow> m_window;
        ImGuiID m_parentViewportID = 0;
    };


    ImGuiPlatform::ImGuiPlatform(Core::PlatformApplication* application, Core::PlatformWindow* mainWindow)
        : m_application(application)
        , m_mainWindow(mainWindow)
    {
        FE_PROFILER_ZONE();

        ImGui::SetAllocatorFunctions(&AllocateImGuiMemory, &FreeImGuiMemory);
        m_context = ImGui::CreateContext();
        ImGui::StyleColorsDark();

        ImGuiIO& io = ImGui::GetIO();
        io.BackendPlatformName = "Ferrum3D";
        io.BackendPlatformUserData = this;
        io.BackendFlags |= kPlatformBackendFlags;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_ViewportsEnable;

        ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();
        platformIO.Platform_CreateWindow = &CreateViewportWindow;
        platformIO.Platform_DestroyWindow = &DestroyViewportWindow;
        platformIO.Platform_ShowWindow = &ShowViewportWindow;
        platformIO.Platform_UpdateWindow = &UpdateViewportWindow;
        platformIO.Platform_SetWindowPos = [](ImGuiViewport* viewport, const ImVec2 position) {
            GetWindow(viewport)->SetPosition(ToVector2Int(position));
        };
        platformIO.Platform_GetWindowPos = [](ImGuiViewport* viewport) {
            return ToImVec2(GetWindow(viewport)->GetPosition());
        };
        platformIO.Platform_SetWindowSize = [](ImGuiViewport* viewport, const ImVec2 size) {
            GetWindow(viewport)->SetSize(ToVector2Int(size));
        };
        platformIO.Platform_GetWindowSize = [](ImGuiViewport* viewport) {
            return ToImVec2(GetWindow(viewport)->GetClientSize());
        };
        platformIO.Platform_SetWindowFocus = [](ImGuiViewport* viewport) {
            GetWindow(viewport)->SetFocus();
        };
        platformIO.Platform_GetWindowFocus = [](ImGuiViewport* viewport) {
            return GetWindow(viewport)->IsFocused();
        };
        platformIO.Platform_GetWindowMinimized = [](ImGuiViewport* viewport) {
            return GetWindow(viewport)->GetShowMode() == Core::PlatformWindowShowMode::kMinimized;
        };
        platformIO.Platform_SetWindowTitle = [](ImGuiViewport* viewport, const char* title) {
            GetWindow(viewport)->SetTitle(title);
        };

        ImGuiViewport* mainViewport = ImGui::GetMainViewport();
        mainViewport->PlatformHandle = mainWindow;
        mainViewport->PlatformHandleRaw = reinterpret_cast<void*>(mainWindow->GetNativeHandle().m_value);

        m_previousFrameTicks = Platform::GetTicks();
    }


    ImGuiPlatform::~ImGuiPlatform()
    {
        FE_PROFILER_ZONE();

        ImGui::SetCurrentContext(m_context);
        ImGui::DestroyPlatformWindows();

        ImGuiIO& io = ImGui::GetIO();
        FE_Assert(io.BackendRendererUserData == nullptr, "Destroy the ImGui renderer backend first");
        ImGui::GetPlatformIO().ClearPlatformHandlers();
        io.BackendPlatformName = nullptr;
        io.BackendPlatformUserData = nullptr;
        io.BackendFlags &= ~kPlatformBackendFlags;

        m_application->SetInputCapture(false, false);
        m_application->SetCursor(Core::PlatformCursor::kArrow);
        ImGui::DestroyContext(m_context);
    }


    void ImGuiPlatform::NewFrame()
    {
        FE_PROFILER_ZONE();

        ImGui::SetCurrentContext(m_context);
        ImGuiIO& io = ImGui::GetIO();

        const uint64_t ticks = Platform::GetTicks();
        const double elapsed = static_cast<double>(ticks - m_previousFrameTicks) * Platform::GetSecondsPerTick();
        io.DeltaTime = Math::Max(static_cast<float>(elapsed), 1.0f / 1000.0f);
        m_previousFrameTicks = ticks;

        io.DisplaySize = ToImVec2(m_mainWindow->GetClientSize());
        io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

        ProcessEvents();
        if (m_areMonitorsDirty)
            UpdateMonitors();

        UpdateCursor();
        ImGui::NewFrame();
    }


    void ImGuiPlatform::EndFrame()
    {
        FE_PROFILER_ZONE();

        ImGui::SetCurrentContext(m_context);
        ImGui::Render();
        ImGui::UpdatePlatformWindows();

        // Applies to the next poll: the application reads input before the next UI frame decides its focus.
        const ImGuiIO& io = ImGui::GetIO();
        m_application->SetInputCapture(io.WantCaptureMouse, io.WantCaptureKeyboard);
    }


    ImGuiPlatform& ImGuiPlatform::Get()
    {
        auto* platform = static_cast<ImGuiPlatform*>(ImGui::GetIO().BackendPlatformUserData);
        FE_Assert(platform != nullptr);
        return *platform;
    }


    Core::PlatformWindow* ImGuiPlatform::GetWindow(ImGuiViewport* viewport)
    {
        auto* window = static_cast<Core::PlatformWindow*>(viewport->PlatformHandle);
        FE_Assert(window != nullptr);
        return window;
    }


    void ImGuiPlatform::CreateViewportWindow(ImGuiViewport* viewport)
    {
        FE_PROFILER_ZONE();

        Core::PlatformWindowDesc desc;
        desc.m_rect = RectInt::FromPosAndSize(ToVector2Int(viewport->Pos), ToVector2Int(viewport->Size));
        desc.m_title = "Untitled";
        desc.m_style = ConvertWindowStyle(viewport->Flags);
        if (ImGuiViewport* parent = ImGui::FindViewportByID(viewport->ParentViewportId))
            desc.m_parent = static_cast<Core::PlatformWindow*>(parent->PlatformHandle);

        auto* data = Memory::DefaultNew<ViewportData>();
        data->m_window = Get().m_application->CreateWindow(desc);
        data->m_parentViewportID = viewport->ParentViewportId;

        viewport->PlatformUserData = data;
        viewport->PlatformHandle = data->m_window.Get();
        viewport->PlatformHandleRaw = reinterpret_cast<void*>(data->m_window->GetNativeHandle().m_value);
    }


    void ImGuiPlatform::DestroyViewportWindow(ImGuiViewport* viewport)
    {
        FE_PROFILER_ZONE();

        if (auto* data = static_cast<ViewportData*>(viewport->PlatformUserData))
            Memory::DefaultDelete(data);

        viewport->PlatformUserData = nullptr;
        viewport->PlatformHandle = nullptr;
        viewport->PlatformHandleRaw = nullptr;
    }


    void ImGuiPlatform::ShowViewportWindow(ImGuiViewport* viewport)
    {
        const bool activate = !(viewport->Flags & ImGuiViewportFlags_NoFocusOnAppearing);
        GetWindow(viewport)->Show(activate ? Core::PlatformWindowShowMode::kShow : Core::PlatformWindowShowMode::kShowNoActivate);
    }


    void ImGuiPlatform::UpdateViewportWindow(ImGuiViewport* viewport)
    {
        auto* data = static_cast<ViewportData*>(viewport->PlatformUserData);
        if (data == nullptr)
            return;

        // Docking and merging toggle decorations and ownership of existing windows.
        data->m_window->UpdateStyle(ConvertWindowStyle(viewport->Flags));
        if (data->m_parentViewportID != viewport->ParentViewportId)
        {
            ImGuiViewport* parent = ImGui::FindViewportByID(viewport->ParentViewportId);
            data->m_window->SetParent(parent ? static_cast<Core::PlatformWindow*>(parent->PlatformHandle) : nullptr);
            data->m_parentViewportID = viewport->ParentViewportId;
        }
    }


    void ImGuiPlatform::ProcessEvents()
    {
        ImGuiIO& io = ImGui::GetIO();
        const bool isMouseAbsolute = (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) != 0;
        const Vector2Int mouseOrigin = isMouseAbsolute ? Vector2Int(0, 0) : m_mainWindow->GetPosition();

        for (const Core::PlatformEvent& event : m_application->GetEvents())
        {
            switch (event.m_type)
            {
            case Core::PlatformEventType::kMonitorsChanged:
                m_areMonitorsDirty = true;
                break;

            case Core::PlatformEventType::kWindowClose:
            case Core::PlatformEventType::kWindowMove:
            case Core::PlatformEventType::kWindowResize:
                if (ImGuiViewport* viewport = ImGui::FindViewportByPlatformHandle(event.m_windowPayload.m_window))
                {
                    viewport->PlatformRequestClose |= event.m_type == Core::PlatformEventType::kWindowClose;
                    viewport->PlatformRequestMove |= event.m_type == Core::PlatformEventType::kWindowMove;
                    viewport->PlatformRequestResize |= event.m_type == Core::PlatformEventType::kWindowResize;
                }
                break;

            case Core::PlatformEventType::kWindowFocus:
            case Core::PlatformEventType::kWindowUnfocus:
                io.AddFocusEvent(event.m_type == Core::PlatformEventType::kWindowFocus);
                break;

            case Core::PlatformEventType::kKeyboardKeyDown:
            case Core::PlatformEventType::kKeyboardKeyUp:
                {
                    const ImGuiKey key = ConvertKey(event.m_keyboardKeyPayload.m_key);
                    if (key == ImGuiKey_None)
                        break;

                    const bool isDown = event.m_type == Core::PlatformEventType::kKeyboardKeyDown;
                    io.AddKeyEvent(key, isDown);

                    // Either side holds a modifier; Windows reports both sides separately.
                    if (const ImGuiKey modifier = GetModifier(key); modifier != ImGuiKey_None)
                        io.AddKeyEvent(modifier, isDown || ImGui::IsKeyDown(GetOtherModifierSide(key)));
                }
                break;

            case Core::PlatformEventType::kKeyboardCharInput:
                io.AddInputCharacterUTF16(static_cast<ImWchar16>(event.m_keyboardCharPayload.m_codepoint));
                break;

            case Core::PlatformEventType::kMouseDown:
            case Core::PlatformEventType::kMouseUp:
                if (event.m_mouseButtonPayload.m_button < ImGuiMouseButton_COUNT)
                {
                    const bool isDown = event.m_type == Core::PlatformEventType::kMouseDown;
                    io.AddMouseButtonEvent(static_cast<int32_t>(event.m_mouseButtonPayload.m_button), isDown);
                }
                break;

            case Core::PlatformEventType::kMouseWheel:
                io.AddMouseWheelEvent(0.0f, event.m_mouseWheelPayload.m_delta);
                break;

            case Core::PlatformEventType::kMouseMove:
                {
                    const Vector2Int position(event.m_mouseMovePayload.m_x, event.m_mouseMovePayload.m_y);
                    io.AddMousePosEvent(static_cast<float>(position.x - mouseOrigin.x),
                                        static_cast<float>(position.y - mouseOrigin.y));
                }
                break;

            case Core::PlatformEventType::kMouseLeave:
                io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
                break;

            default:
                break;
            }
        }
    }


    void ImGuiPlatform::UpdateMonitors()
    {
        ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();
        platformIO.Monitors.resize(0);
        for (const Core::PlatformMonitorInfo& info : m_application->GetMonitors())
        {
            ImGuiPlatformMonitor monitor;
            monitor.MainPos = ToImVec2(info.m_rect.min);
            monitor.MainSize = ToImVec2(info.m_rect.Size());
            monitor.WorkPos = ToImVec2(info.m_workRect.min);
            monitor.WorkSize = ToImVec2(info.m_workRect.Size());
            monitor.DpiScale = info.m_dpiScale;
            monitor.PlatformHandle = reinterpret_cast<void*>(info.m_nativeHandle.m_value);
            platformIO.Monitors.push_back(monitor);
        }

        m_areMonitorsDirty = false;
    }


    void ImGuiPlatform::UpdateCursor()
    {
        const ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
            return;

        const ImGuiMouseCursor cursor = io.MouseDrawCursor ? ImGuiMouseCursor_None : ImGui::GetMouseCursor();
        m_application->SetCursor(ConvertCursor(cursor));
    }
} // namespace FE::Framework
