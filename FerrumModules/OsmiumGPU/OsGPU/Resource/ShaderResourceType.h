#pragma once

namespace FE::GPU
{
    enum class ShaderResourceType
    {
        None,
        ConstantBuffer,
        TextureSRV,
        TextureUAV,
        BufferSRV,
        BufferUAV,
        Sampler,
        InputAttachment
    };
} // namespace FE::GPU
