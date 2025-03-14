#pragma once

namespace FE::Graphics::Core
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
        kInputAttachment,
    };


    constexpr bool IsBufferShaderResource(const ShaderResourceType type)
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


    constexpr bool IsTextureShaderResource(const ShaderResourceType type)
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
} // namespace FE::Graphics::Core
