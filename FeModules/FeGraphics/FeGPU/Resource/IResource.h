#pragma once
#include <FeCore/Memory/Memory.h>

namespace FE::GPU
{
    // clang-format off
    FE_ENUM(BindFlags)
    {
        None             = 0,
        VertexBuffer     = 1 << 0,
        IndexBuffer      = 1 << 1,
        ConstantBuffer   = 1 << 2,
        ShaderResource   = 1 << 3,
        StreamOutput     = 1 << 4,
        RenderTarget     = 1 << 5,
        DepthStencil     = 1 << 6,
        UnorderedAccess  = 1 << 7,
        IndirectDrawArgs = 1 << 8,
        InputAttachment  = 1 << 9
    };
    // clang-format on
} // namespace FE::GPU
