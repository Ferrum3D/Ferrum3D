#pragma once
#include <GPU/Image/ImageSubresource.h>

namespace FE::GPU
{
    enum class ResourceState
    {
        Undefined,
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
        FE_STRUCT_RTTI(ResourceTransitionBarrierDesc, "D4115E22-8C42-4639-9EEB-C53C588AF1D5");

        IImage* Image = nullptr;
        ImageSubresourceRange SubresourceRange;
        ResourceState StateBefore = ResourceState::Undefined;
        ResourceState StateAfter = ResourceState::Undefined;
    };
}
