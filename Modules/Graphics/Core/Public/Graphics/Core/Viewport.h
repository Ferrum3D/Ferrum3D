#pragma once
#include <Core/Math/Rect.h>
#include <Graphics/Core/DeviceObject.h>

namespace FE::Graphics::Core
{
    struct ViewportDesc final
    {
        uint32_t m_width = 0;
        uint32_t m_height = 0;
        uintptr_t m_nativeWindowHandle = 0;

        [[nodiscard]] RectF GetRect() const
        {
            return RectF{ 0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(m_height) };
        }

        static ViewportDesc Create(const RectF rect, const uintptr_t nativeWindowHandle)
        {
            return ViewportDesc{
                .m_width = static_cast<uint32_t>(rect.Width()),
                .m_height = static_cast<uint32_t>(rect.Height()),
                .m_nativeWindowHandle = nativeWindowHandle,
            };
        }

        static ViewportDesc Create(const RectInt rect, const uintptr_t nativeWindowHandle)
        {
            return ViewportDesc{
                .m_width = static_cast<uint32_t>(rect.Width()),
                .m_height = static_cast<uint32_t>(rect.Height()),
                .m_nativeWindowHandle = nativeWindowHandle,
            };
        }
    };


    struct Viewport : public DeviceObject
    {
        FE_RTTI("6190EF25-7202-48E9-A4E7-C5B123881D58");

        virtual void Init(const ViewportDesc& desc) = 0;
        [[nodiscard]] virtual const ViewportDesc& GetDesc() const = 0;

        virtual Texture* GetCurrentColorTarget() = 0;
        virtual void AcquireNextImage() = 0;
        virtual void PrepareBlit() = 0;
        virtual void Present() = 0;
    };
} // namespace FE::Graphics::Core
