#pragma once
#include <FeCore/Math/Rect.h>
#include <FeCore/Memory/Memory.h>
#include <festd/string.h>

namespace FE::Framework::Core
{
    struct PlatformWindow;

    struct NativeWindowHandle final : public TypedHandle<NativeWindowHandle, uint64_t>
    {
    };


    enum class PlatformWindowShowMode : uint32_t
    {
        kShow,
        kShowNoActivate,
        kNormal,
        kMinimized,
        kMaximized,
    };


    enum class PlatformWindowStyleFlags : uint32_t
    {
        kNone = 0,
        kNoDecoration = 1 << 0,
        kNoTaskBarIcon = 1 << 1,
        kTopMost = 1 << 2,
    };

    FE_ENUM_OPERATORS(PlatformWindowStyleFlags);


    struct PlatformWindowDesc final
    {
        RectInt m_rect = RectInt::Zero();
        festd::string_view m_title;
        PlatformWindow* m_parent = nullptr;
        PlatformWindowStyleFlags m_style = PlatformWindowStyleFlags::kNone;
    };


    struct PlatformWindow : public Memory::RefCountedObjectBase
    {
        FE_RTTI_Class(PlatformWindow, "C1F93BE6-5FE7-4E28-A5F6-8A1E9A5F427F");

        virtual NativeWindowHandle GetNativeHandle() const = 0;
        virtual PlatformApplication* GetApplication() const = 0;

        virtual void Show(PlatformWindowShowMode mode) = 0;

        virtual void SetFocus() = 0;
        virtual void SetParent(PlatformWindow* parent) = 0;
        virtual void UpdateStyle(PlatformWindowStyleFlags style) = 0;
        virtual void SetTitle(festd::string_view title) = 0;

        virtual void SetPosition(Vector2Int position) = 0;
        virtual void SetSize(Vector2Int size) = 0;

        virtual bool IsFocused() const = 0;
        virtual PlatformWindowShowMode GetShowMode() const = 0;
        virtual uint32_t GetMonitorIndex() const = 0;

        virtual Vector2Int GetPosition() const = 0;
        virtual Vector2Int GetClientSize() const = 0;
        virtual RectInt GetClientRect() const = 0;
    };
} // namespace FE::Framework::Core
