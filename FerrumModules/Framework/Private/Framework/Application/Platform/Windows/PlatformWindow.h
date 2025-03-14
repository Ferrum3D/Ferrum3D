#pragma once
#include <FeCore/Base/PlatformInclude.h>
#include <FeCore/Logging/Logger.h>
#include <FeCore/Threading/SharedSpinLock.h>
#include <Framework/Application/Core/PlatformWindow.h>
#include <festd/vector.h>

namespace FE::Framework::Windows
{
    struct PlatformApplication;

    struct PlatformWindow final : public Core::PlatformWindow
    {
        FE_RTTI_Class(PlatformWindow, "74E2D079-ADE7-4348-AD82-B1094B3B5171");

        PlatformWindow(Core::PlatformApplication* application, Logger* logger);
        ~PlatformWindow() override;

        void Init(const Core::PlatformWindowDesc& desc);

        Core::NativeWindowHandle GetNativeHandle() const override;
        Core::PlatformApplication* GetApplication() const override;
        void Show(Core::PlatformWindowShowMode showMode) override;
        void SetFocus() override;
        void SetParent(Core::PlatformWindow* parent) override;
        void UpdateStyle(Core::PlatformWindowStyleFlags newStyle) override;
        void SetTitle(festd::string_view title) override;
        void SetPosition(Vector2Int position) override;
        void SetSize(Vector2Int size) override;

        bool IsFocused() const override;
        Core::PlatformWindowShowMode GetShowMode() const override;
        uint32_t GetMonitorIndex() const override;
        Vector2Int GetPosition() const override;
        Vector2Int GetClientSize() const override;
        RectInt GetClientRect() const override;

    private:
        HWND m_hwnd = nullptr;
        PlatformWindow* m_parent = nullptr;
        PlatformApplication* m_application = nullptr;
        Logger* m_logger = nullptr;
        festd::small_vector<PlatformWindow*> m_children;
        Core::PlatformWindowDesc m_desc;
    };


    inline PlatformWindow* ImplCast(Core::PlatformWindow* window)
    {
        return static_cast<PlatformWindow*>(window);
    }


    inline const PlatformWindow* ImplCast(const Core::PlatformWindow* window)
    {
        return static_cast<const PlatformWindow*>(window);
    }
} // namespace FE::Framework::Windows
