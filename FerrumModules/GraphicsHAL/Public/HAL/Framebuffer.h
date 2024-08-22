#pragma once
#include <HAL/DeviceObject.h>

namespace FE::Graphics::HAL
{
    class ImageView;
    class RenderPass;

    struct FramebufferDesc
    {
        festd::span<ImageView*> RenderTargetViews{};
        Rc<RenderPass> RenderPass{};
        uint32_t Width = 0;
        uint32_t Height = 0;
    };

    class Framebuffer : public DeviceObject
    {
    public:
        FE_RTTI_Class(Framebuffer, "CDF97657-3843-4767-AA28-FAEF82702000");

        ~Framebuffer() override = default;

        virtual ResultCode Init(const FramebufferDesc& desc) = 0;

        [[nodiscard]] virtual const FramebufferDesc& GetDesc() const = 0;
    };
} // namespace FE::Graphics::HAL
