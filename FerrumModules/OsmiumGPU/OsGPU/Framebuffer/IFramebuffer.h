#pragma once
#include <FeCore/Containers/ArraySlice.h>

namespace FE::Osmium
{
    class IImageView;
    class IRenderPass;

    struct FramebufferDesc
    {
        FE_RTTI_Base(FramebufferDesc, "0A969932-5EFF-401E-9318-1C77D190E8A9");

        ArraySlice<IImageView*> RenderTargetViews{};
        Rc<IRenderPass> RenderPass{};
        uint32_t Width  = 0;
        uint32_t Height = 0;
    };

    class IFramebuffer : public Memory::RefCountedObjectBase
    {
    public:
        FE_RTTI_Class(IFramebuffer, "CDF97657-3843-4767-AA28-FAEF82702000");

        ~IFramebuffer() override = default;

        [[nodiscard]] virtual const FramebufferDesc& GetDesc() const = 0;
    };
} // namespace FE::Osmium
