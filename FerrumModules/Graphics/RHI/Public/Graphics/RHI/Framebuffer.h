#pragma once
#include <Graphics/RHI/DeviceObject.h>

namespace FE::Graphics::RHI
{
    struct ImageView;
    struct RenderPass;

    struct FramebufferDesc final
    {
        festd::span<ImageView* const> m_renderTargetViews;
        Rc<RenderPass> m_renderPass;
        uint32_t m_width = 0;
        uint32_t m_height = 0;
    };

    struct Framebuffer : public DeviceObject
    {
        FE_RTTI_Class(Framebuffer, "CDF97657-3843-4767-AA28-FAEF82702000");

        ~Framebuffer() override = default;

        virtual ResultCode Init(const FramebufferDesc& desc) = 0;

        [[nodiscard]] virtual const FramebufferDesc& GetDesc() const = 0;
    };
} // namespace FE::Graphics::RHI
