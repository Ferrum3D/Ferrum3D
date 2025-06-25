#pragma once
#include <FeCore/Memory/PoolAllocator.h>
#include <Framework/Application/Core/PlatformApplication.h>
#include <Framework/Application/Platform/Windows/PlatformWindow.h>

namespace FE::Framework::Windows
{
    inline constexpr const WCHAR* kWindowClassName = L"FerrumWindowClass";


    struct PlatformApplication final : public Core::PlatformApplication
    {
        FE_RTTI_Class(PlatformApplication, "70A9DF12-1D5D-4D0A-8C24-C014E3963121");

        PlatformApplication();
        ~PlatformApplication() override;

        Core::AlertResponse ShowAlert(Core::AlertKind kind, festd::string_view title, festd::string_view message,
                                      Core::AlertButtons buttons) override;

        Core::PlatformWindow* CreateWindow(const Core::PlatformWindowDesc& desc) override;

        void PollEvents() override;
        bool IsCloseRequested() const override;
        festd::span<const Core::PlatformEvent> GetEvents() const override;
        festd::span<const Core::PlatformMonitorInfo> GetMonitors() const override;
        festd::span<Core::PlatformWindow* const> GetWindows() const override;
        Core::PlatformWindow* GetMainWindow() const override;

    private:
        friend PlatformWindow;

        void UnregisterWindow(PlatformWindow* window)
        {
            m_windows.erase(eastl::remove(m_windows.begin(), m_windows.end(), window), m_windows.end());
        }

        static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
        void UpdateKeyboardCodePage();
        void UpdateMonitors();

        enum class MouseTrackArea
        {
            kNone = 0,
            kClient = 1,
            kNonClient = 2,
        };

        HWND m_mouseTrackWindow = nullptr;
        MouseTrackArea m_mouseTrackArea = MouseTrackArea::kNone;
        uint32_t m_mouseButtonsDown = Constants::kMaxU32;

        uint32_t m_keyboardCodePage = 0;
        bool m_closeRequested = false;
        Memory::Pool<PlatformWindow> m_windowPool{ "PlatformWindowPool" };
        festd::inline_vector<Core::PlatformWindow*> m_windows;
        festd::vector<Core::PlatformEvent> m_events;
        festd::vector<Core::PlatformMonitorInfo> m_monitors;
    };


    inline PlatformApplication* ImplCast(Core::PlatformApplication* app)
    {
        return static_cast<PlatformApplication*>(app);
    }

    inline const PlatformApplication* ImplCast(const Core::PlatformApplication* app)
    {
        return static_cast<const PlatformApplication*>(app);
    }
} // namespace FE::Framework::Windows
