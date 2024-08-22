#pragma once

namespace FE::Graphics::HAL
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
} // namespace FE::Graphics::HAL
