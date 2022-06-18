#pragma once
#include <FeCore/Containers/IByteBuffer.h>
#include <GPU/RenderPass/IRenderPass.h>

namespace FE::GPU
{
    struct RenderPassDescBinding
    {
        IByteBuffer* Subpasses;
        IByteBuffer* Attachments;
        IByteBuffer* SubpassDependencies;
    };

    struct SubpassDescBinding
    {
        IByteBuffer* InputAttachments;
        IByteBuffer* RenderTargetAttachments;
        IByteBuffer* PreserveAttachments;
        SubpassAttachment DepthStencilAttachment;
    };
} // namespace FE::GPU
