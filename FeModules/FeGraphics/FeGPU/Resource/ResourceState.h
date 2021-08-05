#pragma once
#include <FeGPU/Image/ImageSubresource.h>

namespace FE::GPU
{
    enum class ResourceState
    {
        None,
        Common,
        VertexAndConstantBuffer,
        IndexBuffer,
        RenderTarget,
        UnorderedAccess,
        DepthWrite,
        DepthRead,
        NonPixelShaderResource,
        PixelShaderResource,
        StreamOut,
        IndirectArgument,
        CopyDest,
        CopySource,
        GenericRead,
        AllShaderResource,
        Present,
        Predication
    };

    class IResource;

    struct ResourceTransitionBarrierDesc
    {
        IResource* Resource;
        ImageSubresourceRange SubresourceRange;
        ResourceState StateBefore;
        ResourceState StateAfter;
    };
}
