#pragma once
#include <FeCore/Base/Base.h>
#include <FeCore/Containers/ByteBuffer.h>

namespace FE::Osmium
{
    class IRenderPass;
    class IImageView;

    struct FramebufferDescBinding
    {
        ByteBuffer RenderTargetViews;
        IRenderPass* RenderPass;
        uint32_t Width;
        uint32_t Height;
    };
} // namespace FE::Osmium
