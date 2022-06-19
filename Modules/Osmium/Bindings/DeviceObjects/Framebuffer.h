#pragma once
#include <FeCore/Containers/IByteBuffer.h>
#include <GPU/Framebuffer/IFramebuffer.h>

namespace FE::GPU
{
    struct FramebufferDescBinding
    {
        IByteBuffer* RenderTargetViews;
        IRenderPass* RenderPass;
        UInt32 Width;
        UInt32 Height;
    };
}
