#pragma once
#include <FeCore/Memory/Memory.h>
#include <festd/string.h>

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


    struct PlatformWindow;
    struct PlatformWindowDesc;
    struct PlatformEvent;
    struct PlatformMonitorInfo;

    struct PlatformApplication : public Memory::RefCountedObjectBase
    {
        FE_RTTI_Class(PlatformApplication, "63770294-E7A2-4CDD-866C-BBC49EC93214");

        virtual AlertResponse ShowAlert(AlertKind kind, festd::string_view title, festd::string_view message,
                                        AlertButtons buttons = AlertButtons::kOkCancel) = 0;

        virtual PlatformWindow* CreateWindow(const PlatformWindowDesc& desc) = 0;

        virtual void PollEvents() = 0;
        virtual bool IsCloseRequested() const = 0;
        virtual festd::span<const PlatformEvent> GetEvents() const = 0;
        virtual festd::span<const PlatformMonitorInfo> GetMonitors() const = 0;
        virtual festd::span<PlatformWindow* const> GetWindows() const = 0;
        virtual PlatformWindow* GetMainWindow() const = 0;
    };
} // namespace FE::Framework::Core
