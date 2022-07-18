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
        UInt32 Width;
        UInt32 Height;
    };
} // namespace FE::Osmium
