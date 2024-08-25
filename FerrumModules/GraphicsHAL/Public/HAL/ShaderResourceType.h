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


    inline constexpr bool IsBufferShaderResource(ShaderResourceType type)
    {
        switch (type)
        {
        case ShaderResourceType::ConstantBuffer:
        case ShaderResourceType::BufferSRV:
        case ShaderResourceType::BufferUAV:
            return true;
        default:
            return false;
        }
    }


    inline constexpr bool IsTextureShaderResource(ShaderResourceType type)
    {
        switch (type)
        {
        case ShaderResourceType::TextureSRV:
        case ShaderResourceType::TextureUAV:
            return true;
        default:
            return false;
        }
    }
} // namespace FE::Graphics::HAL
