#pragma once
#include <FeCore/Containers/List.h>
#include <FeCore/Memory/SharedPtr.h>

namespace FE::GPU
{
    class IImageView;
    class IRenderPass;

    struct FramebufferDesc
    {
        FE_STRUCT_RTTI(FramebufferDesc, "0A969932-5EFF-401E-9318-1C77D190E8A9");

        List<Shared<IImageView>> RenderTargetViews{};
        Shared<IRenderPass> RenderPass{};
        UInt32 Width  = 0;
        UInt32 Height = 0;
    };

    class IFramebuffer : public IObject
    {
    public:
        FE_CLASS_RTTI(IFramebuffer, "CDF97657-3843-4767-AA28-FAEF82702000");

        ~IFramebuffer() override = default;

        [[nodiscard]] virtual const FramebufferDesc& GetDesc() const = 0;
    };
} // namespace FE::GPU
