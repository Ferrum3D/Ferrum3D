#pragma once
#include <Core/Memory/PoolAllocator.h>
#include <Framework/Application/Core/PlatformApplication.h>
#include <Framework/Application/Platform/Windows/PlatformWindow.h>
#include <Framework/Input/Desktop/Backend.h>

namespace FE::Framework::Windows
{
    inline constexpr const WCHAR* kWindowClassName = L"FerrumWindowClass";


    struct PlatformApplication final : public Core::PlatformApplication
    {
        FE_RTTI("70A9DF12-1D5D-4D0A-8C24-C014E3963121");

        PlatformApplication();
        ~PlatformApplication() override;

        Core::AlertResponse ShowAlert(Core::AlertKind kind, festd::string_view title, festd::string_view message,
                                      Core::AlertButtons buttons) override;

        Core::PlatformWindow* CreateWindow(const Core::PlatformWindowDesc& desc) override;

        void PollEvents() override;
        [[nodiscard]] const Input::Core::Backend& GetInputBackend() const override
        {
            return m_inputBackend;
        }

        void SetCursor(Core::PlatformCursor cursor) override;
        void SetInputCapture(bool isMouseCaptured, bool isKeyboardCaptured) override;
        bool IsCloseRequested() const override;
        festd::span<const Core::PlatformEvent> GetEvents() const override;
        festd::span<const Core::PlatformMonitorInfo> GetMonitors() const override;
        festd::span<Core::PlatformWindow* const> GetWindows() const override;
        Core::PlatformWindow* GetMainWindow() const override;

    private:
        friend PlatformWindow;

        void DoRelease() override
        {
            Memory::DefaultDelete(this);
        }

        void UnregisterWindow(PlatformWindow* window)
        {
            const auto found = festd::find(m_windows.begin(), m_windows.end(), window);
            if (found != m_windows.end())
                m_windows.erase(found);
        }

        static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
        void RecordEvent(Core::PlatformEvent event);
        void UpdateKeyboardCodePage();
        void UpdateMonitors();
        void ApplyCursor() const;
        [[nodiscard]] festd::span<const Core::PlatformEvent> FilterCapturedInput();

        enum class MouseTrackArea
        {
            kNone = 0,
            kClient = 1,
            kNonClient = 2,
        };

        Input::Desktop::Backend m_inputBackend;
        double m_previousPollSeconds = 0;
        const MSG* m_pendingMessage = nullptr;
        festd::optional<double> m_eventTimestamp;
        HWND m_mouseTrackWindow = nullptr;
        MouseTrackArea m_mouseTrackArea = MouseTrackArea::kNone;
        uint32_t m_mouseButtonsDown = Constants::kMaxU32;

        uint32_t m_keyboardCodePage = 0;
        bool m_closeRequested = false;
        bool m_isMouseCaptured = false;
        bool m_isKeyboardCaptured = false;
        Core::PlatformCursor m_cursor = Core::PlatformCursor::kArrow;
        festd::inline_vector<Core::PlatformWindow*> m_windows;
        festd::vector<Core::PlatformEvent> m_events;
        //! Prefix of m_events returned by the latest poll; later entries were recorded between polls.
        uint32_t m_deliveredEventCount = 0;
        festd::vector<Core::PlatformEvent> m_inputEvents;
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
