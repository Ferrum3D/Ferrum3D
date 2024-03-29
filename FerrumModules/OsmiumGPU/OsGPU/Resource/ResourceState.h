#pragma once
#include <OsGPU/Image/ImageSubresource.h>

namespace FE::Osmium
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
        TransferWrite,
        TransferRead,
        Present,
        Automatic
    };

    class IImage;
    class IBuffer;

    struct ImageBarrierDesc
    {
        FE_STRUCT_RTTI(ImageBarrierDesc, "D4115E22-8C42-4639-9EEB-C53C588AF1D5");

        IImage* Image = nullptr;
        ImageSubresourceRange SubresourceRange;
        ResourceState StateBefore = ResourceState::Automatic;
        ResourceState StateAfter  = ResourceState::Undefined;
    };

    struct BufferBarrierDesc
    {
        FE_STRUCT_RTTI(BufferBarrierDesc, "88526570-44FF-4013-8A04-B513E42CB2DA");

        IBuffer* Buffer           = nullptr;
        UInt64 Offset             = 0;
        UInt64 Size               = 0;
        ResourceState StateBefore = ResourceState::Automatic;
        ResourceState StateAfter  = ResourceState::Undefined;
    };
} // namespace FE::Osmium
