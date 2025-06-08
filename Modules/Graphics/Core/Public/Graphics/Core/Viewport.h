#pragma once
#include <Graphics/Core/DeviceObject.h>
#include <Graphics/Core/ImageFormat.h>
#include <Graphics/Core/RenderTarget.h>

namespace FE::Graphics::Core
{
    constexpr auto kMainDepthTargetFormat = Format::kD32_SFLOAT_S8_UINT;


    struct ViewportDesc final
    {
        uint32_t m_width = 0;
        uint32_t m_height = 0;
        uint64_t m_nativeWindowHandle = 0;
    };


    struct Viewport : public DeviceObject
    {
        FE_RTTI_Class(Viewport, "6190EF25-7202-48E9-A4E7-C5B123881D58");

        virtual void Init(const ViewportDesc& desc) = 0;
        [[nodiscard]] virtual const ViewportDesc& GetDesc() const = 0;

        virtual Format GetColorTargetFormat() = 0;
        virtual Format GetDepthTargetFormat() = 0;

        virtual RenderTarget* GetCurrentColorTarget() = 0;
        virtual RenderTarget* GetDepthTarget() = 0;
    };
} // namespace FE::Graphics::Core
