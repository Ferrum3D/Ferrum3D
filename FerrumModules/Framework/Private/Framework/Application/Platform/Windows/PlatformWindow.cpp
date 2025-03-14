#include <FeCore/Strings/Encoding.h>
#include <FeCore/Threading/Thread.h>
#include <Framework/Application/Core/PlatformMonitor.h>
#include <Framework/Application/Platform/Windows/PlatformApplication.h>
#include <Framework/Application/Platform/Windows/PlatformWindow.h>

namespace FE::Framework::Windows
{
    namespace
    {
        RectInt ConvertRect(const RECT rect)
        {
            return festd::bit_cast<RectInt>(rect);
        }


        RECT ConvertRect(const RectInt rect)
        {
            return festd::bit_cast<RECT>(rect);
        }


        void ConvertWindowStyle(const Core::PlatformWindowStyleFlags srcStyle, DWORD& style, DWORD& exStyle)
        {
            style = Bit::AllSet(srcStyle, Core::PlatformWindowStyleFlags::kNoDecoration) ? WS_POPUP : WS_OVERLAPPEDWINDOW;
            exStyle = Bit::AllSet(srcStyle, Core::PlatformWindowStyleFlags::kNoTaskBarIcon) ? WS_EX_TOOLWINDOW : WS_EX_APPWINDOW;

            if (Bit::AllSet(srcStyle, Core::PlatformWindowStyleFlags::kTopMost))
                exStyle |= WS_EX_TOPMOST;
        }
    } // namespace


    PlatformWindow::PlatformWindow(Core::PlatformApplication* application, Logger* logger)
        : m_application(ImplCast(application))
        , m_logger(logger)
    {
        FE_Assert(Threading::IsMainThread());
    }


    PlatformWindow::~PlatformWindow()
    {
        FE_Assert(Threading::IsMainThread());

        PlatformWindow* mainWindow = ImplCast(m_application->GetMainWindow());
        const bool isMainWindow = m_hwnd == mainWindow->m_hwnd;

        if (m_hwnd)
        {
            if (GetCapture() == m_hwnd)
            {
                ReleaseCapture();

                if (!isMainWindow)
                    SetCapture(mainWindow->m_hwnd);
            }

            DestroyWindow(m_hwnd);
            m_hwnd = nullptr;
        }

        if (isMainWindow)
        {
            FE_Assert(m_children.empty());
        }
        else
        {
            for (PlatformWindow* child : m_children)
                mainWindow->m_children.push_back(child);
        }

        if (m_parent)
        {
            m_parent->m_children.erase(eastl::remove(m_parent->m_children.begin(), m_parent->m_children.end(), this),
                                       m_parent->m_children.end());
        }

        m_application->UnregisterWindow(this);
    }


    void PlatformWindow::Init(const Core::PlatformWindowDesc& desc)
    {
        FE_PROFILER_ZONE();

        FE_Assert(Threading::IsMainThread());
        FE_Assert(!m_hwnd, "Window already initialized");

        m_desc = desc;
        m_parent = ImplCast(desc.m_parent);

        // TODO: thread temp allocator

        const Str::Utf8ToUtf16 windowName{ desc.m_title.data(), desc.m_title.size() };

        const HWND parentHandle = m_parent ? m_parent->m_hwnd : nullptr;

        DWORD style, exStyle;
        ConvertWindowStyle(desc.m_style, style, exStyle);

        RECT windowRect = ConvertRect(desc.m_rect);
        FE_Verify(AdjustWindowRectEx(&windowRect, style, false, exStyle), "AdjustWindowRectEx failed");

        m_hwnd = CreateWindowExW(exStyle,
                                 kWindowClassName,
                                 windowName.ToWideString(),
                                 style,
                                 windowRect.left,
                                 windowRect.top,
                                 windowRect.right - windowRect.left,
                                 windowRect.bottom - windowRect.top,
                                 parentHandle,
                                 nullptr,
                                 GetModuleHandleW(nullptr),
                                 nullptr);

        FE_Assert(m_hwnd, "Failed to create window");

        SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
        UpdateWindow(m_hwnd);
    }


    Core::NativeWindowHandle PlatformWindow::GetNativeHandle() const
    {
        return Core::NativeWindowHandle{ reinterpret_cast<uintptr_t>(m_hwnd) };
    }


    Core::PlatformApplication* PlatformWindow::GetApplication() const
    {
        return m_application;
    }


    void PlatformWindow::Show(const Core::PlatformWindowShowMode showMode)
    {
        FE_PROFILER_ZONE();

        FE_Assert(Threading::IsMainThread());
        FE_AssertDebug(m_hwnd);

        if (m_parent)
            SetWindowLongPtrW(m_hwnd, GWLP_HWNDPARENT, 0);

        switch (showMode)
        {
        case Core::PlatformWindowShowMode::kShow:
            ShowWindow(m_hwnd, SW_SHOW);
            break;
        case Core::PlatformWindowShowMode::kShowNoActivate:
            ShowWindow(m_hwnd, SW_SHOWNA);
            break;
        case Core::PlatformWindowShowMode::kNormal:
            ShowWindow(m_hwnd, SW_SHOWNORMAL);
            break;
        case Core::PlatformWindowShowMode::kMaximized:
            ShowWindow(m_hwnd, SW_SHOWMAXIMIZED);
            break;
        case Core::PlatformWindowShowMode::kMinimized:
            ShowWindow(m_hwnd, SW_SHOWMINIMIZED);
            break;
        default:
            FE_DebugBreak();
            break;
        }

        if (m_parent)
            SetWindowLongPtrW(m_hwnd, GWLP_HWNDPARENT, reinterpret_cast<LONG_PTR>(m_parent->m_hwnd));
    }


    void PlatformWindow::SetFocus()
    {
        FE_Assert(Threading::IsMainThread());
        FE_AssertDebug(m_hwnd);

        ::BringWindowToTop(m_hwnd);
        ::SetForegroundWindow(m_hwnd);
        ::SetFocus(m_hwnd);
    }


    void PlatformWindow::SetParent(Core::PlatformWindow* parent)
    {
        FE_Assert(Threading::IsMainThread());
        FE_AssertDebug(m_hwnd);

        if (parent != m_parent)
        {
            m_parent = ImplCast(parent);
            SetWindowLongPtrW(m_hwnd, GWLP_HWNDPARENT, reinterpret_cast<LONG_PTR>(m_parent->m_hwnd));
        }
    }


    void PlatformWindow::UpdateStyle(const Core::PlatformWindowStyleFlags newStyle)
    {
        FE_PROFILER_ZONE();

        FE_Assert(Threading::IsMainThread());
        FE_AssertDebug(m_hwnd);

        if (newStyle == m_desc.m_style)
            return;

        DWORD style, exStyle;
        ConvertWindowStyle(newStyle, style, exStyle);

        const bool wasTopMost = Bit::AllSet(m_desc.m_style, Core::PlatformWindowStyleFlags::kTopMost);
        const bool isTopMost = Bit::AllSet(newStyle, Core::PlatformWindowStyleFlags::kTopMost);

        const bool topMostChanged = wasTopMost != isTopMost;
        const HWND insertAfter = topMostChanged ? (isTopMost ? HWND_TOPMOST : HWND_NOTOPMOST) : 0;
        const UINT swpFlag = topMostChanged ? 0 : SWP_NOZORDER;

        SetWindowLongW(m_hwnd, GWL_STYLE, static_cast<LONG>(style));
        SetWindowLongW(m_hwnd, GWL_EXSTYLE, static_cast<LONG>(exStyle));

        RECT rect = ConvertRect(m_desc.m_rect);
        AdjustWindowRectEx(&rect, style, FALSE, exStyle);

        SetWindowPos(m_hwnd,
                     insertAfter,
                     rect.left,
                     rect.top,
                     rect.right - rect.left,
                     rect.bottom - rect.top,
                     swpFlag | SWP_NOACTIVATE | SWP_FRAMECHANGED);
        ShowWindow(m_hwnd, SW_SHOWNA);

        m_desc.m_style = newStyle;
    }


    void PlatformWindow::SetTitle(const festd::string_view title)
    {
        FE_PROFILER_ZONE();

        FE_Assert(Threading::IsMainThread());
        FE_AssertDebug(m_hwnd);

        // TODO: thread temp allocator

        m_desc.m_title = title;

        const Str::Utf8ToUtf16 windowTitle{ title.data(), title.size() };
        SetWindowTextW(m_hwnd, windowTitle.ToWideString());
    }


    void PlatformWindow::SetPosition(const Vector2Int position)
    {
        FE_Assert(Threading::IsMainThread());
        FE_AssertDebug(m_hwnd);

        const DWORD style = GetWindowLongW(m_hwnd, GWL_STYLE);
        const DWORD exStyle = GetWindowLongW(m_hwnd, GWL_EXSTYLE);

        RECT rect;
        rect.left = rect.right = position.x;
        rect.top = rect.bottom = position.y;

        AdjustWindowRectEx(&rect, style, FALSE, exStyle);
        SetWindowPos(m_hwnd, nullptr, rect.left, rect.top, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);

        const Vector2Int diff = Vector2Int{ rect.left, rect.top } - m_desc.m_rect.min;
        m_desc.m_rect = Math::Offset(m_desc.m_rect, diff);
    }


    void PlatformWindow::SetSize(const Vector2Int size)
    {
        FE_Assert(Threading::IsMainThread());
        FE_AssertDebug(m_hwnd);

        const DWORD style = GetWindowLongW(m_hwnd, GWL_STYLE);
        const DWORD exStyle = GetWindowLongW(m_hwnd, GWL_EXSTYLE);

        RECT rect;
        rect.left = rect.top = 0;
        rect.right = size.x;
        rect.bottom = size.y;

        AdjustWindowRectEx(&rect, style, FALSE, exStyle);

        const int32_t width = rect.right - rect.left;
        const int32_t height = rect.bottom - rect.top;
        SetWindowPos(m_hwnd, nullptr, 0, 0, width, height, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);

        const Vector2Int pos = m_desc.m_rect.min;
        m_desc.m_rect = Math::Offset(ConvertRect(rect), pos);
    }


    bool PlatformWindow::IsFocused() const
    {
        FE_Assert(Threading::IsMainThread());
        FE_AssertDebug(m_hwnd);

        return GetForegroundWindow() == m_hwnd;
    }


    Core::PlatformWindowShowMode PlatformWindow::GetShowMode() const
    {
        FE_Assert(Threading::IsMainThread());
        FE_AssertDebug(m_hwnd);

        if (IsIconic(m_hwnd))
            return Core::PlatformWindowShowMode::kMinimized;

        if (IsZoomed(m_hwnd))
            return Core::PlatformWindowShowMode::kMaximized;

        return Core::PlatformWindowShowMode::kNormal;
    }


    uint32_t PlatformWindow::GetMonitorIndex() const
    {
        FE_Assert(Threading::IsMainThread());
        FE_AssertDebug(m_hwnd);

        const HMONITOR hMonitor = MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTONEAREST);
        assert(hMonitor);

        const Core::NativeMonitorHandle monitor{ reinterpret_cast<uint64_t>(hMonitor) };
        const festd::span<const Core::PlatformMonitorInfo> monitors = m_application->GetMonitors();
        for (uint32_t monitorIndex = 0; monitorIndex < monitors.size(); ++monitorIndex)
        {
            if (monitors[monitorIndex].m_nativeHandle == monitor)
                return monitorIndex;
        }

        return kInvalidIndex;
    }


    Vector2Int PlatformWindow::GetPosition() const
    {
        FE_Assert(Threading::IsMainThread());
        FE_AssertDebug(m_hwnd);

        POINT point{ 0, 0 };
        ClientToScreen(m_hwnd, &point);
        return Vector2Int{ point.x, point.y };
    }


    Vector2Int PlatformWindow::GetClientSize() const
    {
        FE_Assert(Threading::IsMainThread());
        FE_AssertDebug(m_hwnd);

        return GetClientRect().Size();
    }


    RectInt PlatformWindow::GetClientRect() const
    {
        FE_Assert(Threading::IsMainThread());
        FE_AssertDebug(m_hwnd);

        RECT rect;
        ::GetClientRect(m_hwnd, &rect);
        return ConvertRect(rect);
    }
} // namespace FE::Framework::Windows
