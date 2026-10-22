#pragma once
#include <Core/Memory/Memory.h>
#include <festd/string.h>

namespace FE::Input::Core
{
    struct Backend;
}

namespace FE::Framework::Core
{
    enum class AlertKind : uint32_t
    {
        kInformation,
        kWarning,
        kError,
        kQuestion,
    };


    enum class AlertButtons : uint32_t
    {
        kOk,
        kOkCancel,
        kYesNo,
    };


    enum class AlertResponse : uint32_t
    {
        kNone,
        kOk,
        kCancel,
        kYes,
        kNo,
    };


    enum class PlatformCursor : uint32_t
    {
        kHidden,
        kArrow,
        kTextInput,
        kResizeAll,
        kResizeNS,
        kResizeEW,
        kResizeNESW,
        kResizeNWSE,
        kHand,
        kWait,
        kProgress,
        kNotAllowed,
    };


    struct PlatformWindow;
    struct PlatformWindowDesc;
    struct PlatformEvent;
    struct PlatformMonitorInfo;

    struct PlatformApplication : public Memory::RefCountedObjectBase
    {
        FE_RTTI("63770294-E7A2-4CDD-866C-BBC49EC93214");

        virtual AlertResponse ShowAlert(AlertKind kind, festd::string_view title, festd::string_view message,
                                        AlertButtons buttons = AlertButtons::kOkCancel) = 0;

        virtual PlatformWindow* CreateWindow(const PlatformWindowDesc& desc) = 0;

        virtual void PollEvents() = 0;
        [[nodiscard]] virtual const Input::Core::Backend& GetInputBackend() const = 0;

        //! @brief Select the cursor shown over the client area of application windows.
        virtual void SetCursor(PlatformCursor cursor) = 0;

        //! @brief Withhold presses, wheel and text events from the input backend while another consumer, such as UI, owns them.
        //!
        //! Releases and mouse movement always reach the backend, so held controls cannot get stuck. GetEvents is unaffected.
        virtual void SetInputCapture(bool isMouseCaptured, bool isKeyboardCaptured) = 0;

        virtual bool IsCloseRequested() const = 0;

        virtual festd::span<const PlatformEvent> GetEvents() const = 0;
        virtual festd::span<const PlatformMonitorInfo> GetMonitors() const = 0;
        virtual festd::span<PlatformWindow* const> GetWindows() const = 0;

        virtual PlatformWindow* GetMainWindow() const = 0;

        static void Init();
        static void Shutdown();

        static PlatformApplication& Get();
    };
} // namespace FE::Framework::Core
