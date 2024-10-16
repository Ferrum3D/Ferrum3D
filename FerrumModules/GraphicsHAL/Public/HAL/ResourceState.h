﻿#pragma once
#include <HAL/ImageSubresource.h>

namespace FE::Graphics::HAL
{
    enum class ResourceState
    {
        kUndefined,
        kCommon,
        kVertexBuffer,
        kConstantBuffer,
        kIndexBuffer,
        kRenderTarget,
        kUnorderedAccess,
        kDepthWrite,
        kDepthRead,
        kShaderResource,
        kIndirectArgument,
        kTransferWrite,
        kTransferRead,
        kPresent,
        kAutomatic
    };


    class Image;
    class Buffer;

    struct ImageBarrierDesc
    {
        FE_RTTI_Base(ImageBarrierDesc, "D4115E22-8C42-4639-9EEB-C53C588AF1D5");

        Image* Image = nullptr;
        ImageSubresourceRange SubresourceRange;
        ResourceState StateBefore = ResourceState::kAutomatic;
        ResourceState StateAfter = ResourceState::kUndefined;
    };


    struct BufferBarrierDesc
    {
        FE_RTTI_Base(BufferBarrierDesc, "88526570-44FF-4013-8A04-B513E42CB2DA");

        Buffer* Buffer = nullptr;
        uint64_t Offset = 0;
        uint64_t Size = 0;
        ResourceState StateBefore = ResourceState::kAutomatic;
        ResourceState StateAfter = ResourceState::kUndefined;
    };
} // namespace FE::Graphics::HAL
