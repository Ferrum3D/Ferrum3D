#pragma once

namespace FE::Graphics::RHI
{
    enum class ShaderResourceType : uint32_t
    {
        kNone,
        kConstantBuffer,
        kTextureSRV,
        kTextureUAV,
        kBufferSRV,
        kBufferUAV,
        kSampler,
        kInputAttachment
    };


    inline constexpr bool IsBufferShaderResource(ShaderResourceType type)
    {
        switch (type)
        {
        case ShaderResourceType::kConstantBuffer:
        case ShaderResourceType::kBufferSRV:
        case ShaderResourceType::kBufferUAV:
            return true;
        default:
            return false;
        }
    }


    inline constexpr bool IsTextureShaderResource(ShaderResourceType type)
    {
        switch (type)
        {
        case ShaderResourceType::kTextureSRV:
        case ShaderResourceType::kTextureUAV:
            return true;
        default:
            return false;
        }
    }
} // namespace FE::Graphics::RHI
