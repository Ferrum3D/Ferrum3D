#pragma once
#include <OsGPU/RenderPass/IRenderPass.h>
#include <FeCore/Containers/ByteBuffer.h>

namespace FE::Osmium
{
    struct RenderPassDescBinding
    {
        ByteBuffer Subpasses;
        ByteBuffer Attachments;
        ByteBuffer SubpassDependencies;
    };

    struct SubpassDescBinding
    {
        ByteBuffer InputAttachments;
        ByteBuffer RenderTargetAttachments;
        ByteBuffer MSAAResolveAttachments;
        ByteBuffer PreserveAttachments;
        SubpassAttachment DepthStencilAttachment;
    };
} // namespace FE::Osmium
