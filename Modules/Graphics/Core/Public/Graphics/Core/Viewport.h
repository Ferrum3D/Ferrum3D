#pragma once
#include <FeCore/Math/Rect.h>
#include <Graphics/Core/DeviceObject.h>

namespace FE::Graphics::Core
{
    struct ViewportDesc final
    {
        uint32_t m_width = 0;
        uint32_t m_height = 0;
        uint64_t m_nativeWindowHandle = 0;

        [[nodiscard]] RectF GetRect() const
        {
            return RectF{ 0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(m_height) };
        }
    };


    struct Viewport : public DeviceObject
    {
        FE_RTTI("6190EF25-7202-48E9-A4E7-C5B123881D58");

        virtual void Init(const ViewportDesc& desc) = 0;
        [[nodiscard]] virtual const ViewportDesc& GetDesc() const = 0;

        virtual Texture* GetCurrentColorTarget() = 0;
        virtual void AcquireNextImage() = 0;
        virtual void Present() = 0;
    };
} // namespace FE::Graphics::Core
