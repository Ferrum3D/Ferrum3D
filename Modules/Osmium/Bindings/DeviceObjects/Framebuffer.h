#pragma once
#include <FeCore/Base/Base.h>

namespace FE
{
    class IByteBuffer;
}

namespace FE::GPU
{
    class IRenderPass;

    struct FramebufferDescBinding
    {
        IByteBuffer* RenderTargetViews;
        IRenderPass* RenderPass;
        UInt32 Width;
        UInt32 Height;
    };
} // namespace FE::GPU
