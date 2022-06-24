#pragma once
#include <OsGPU/RenderPass/IRenderPass.h>

namespace FE
{
    class IByteBuffer;
}

namespace FE::Osmium
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
} // namespace FE::Osmium
