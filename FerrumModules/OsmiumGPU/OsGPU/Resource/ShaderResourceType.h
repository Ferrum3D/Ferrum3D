#pragma once

namespace FE::Osmium
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
} // namespace FE::Osmium
