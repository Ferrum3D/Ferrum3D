#pragma once
#include <FeGPU/Image/ImageSubresource.h>

namespace FE::GPU
{
    enum class ResourceState
    {
        None,
        Common,
        VertexBuffer,
        ConstantBuffer,
        IndexBuffer,
        RenderTarget,
        UnorderedAccess,
        DepthWrite,
        DepthRead,
        ShaderResource,
        IndirectArgument,
        CopyDest,
        CopySource,
        Present
    };

    class IImage;

    struct ResourceTransitionBarrierDesc
    {
        IImage* Image;
        ImageSubresourceRange SubresourceRange;
        ResourceState StateBefore;
        ResourceState StateAfter;
    };
}
