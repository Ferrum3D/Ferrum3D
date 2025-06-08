#include <Graphics/Core/Vulkan/Device.h>
#include <Graphics/Core/Vulkan/PipelineStates.h>
#include <Graphics/Core/Vulkan/Sampler.h>

namespace FE::Graphics::Vulkan
{
    namespace
    {
        VkSamplerAddressMode Translate(const Core::SamplerAddressMode mode)
        {
            switch (mode)
            {
            case Core::SamplerAddressMode::kWrap:
                return VK_SAMPLER_ADDRESS_MODE_REPEAT;
            case Core::SamplerAddressMode::kMirror:
                return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
            case Core::SamplerAddressMode::kClamp:
                return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            case Core::SamplerAddressMode::kBorder:
                return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
            case Core::SamplerAddressMode::kMirrorOnce:
                return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
            default:
                FE_DebugBreak();
                return VK_SAMPLER_ADDRESS_MODE_REPEAT;
            }
        }


        VkFilter Translate(const Core::SamplerFilter filter)
        {
            switch (filter)
            {
            case Core::SamplerFilter::kPoint:
                return VK_FILTER_NEAREST;
            case Core::SamplerFilter::kLinear:
                return VK_FILTER_LINEAR;
            default:
                FE_DebugBreak();
                return VK_FILTER_NEAREST;
            }
        }


        VkSamplerMipmapMode TranslateMipmapMode(const Core::SamplerFilter filter)
        {
            switch (filter)
            {
            case Core::SamplerFilter::kPoint:
                return VK_SAMPLER_MIPMAP_MODE_NEAREST;
            case Core::SamplerFilter::kLinear:
                return VK_SAMPLER_MIPMAP_MODE_LINEAR;
            default:
                FE_DebugBreak();
                return VK_SAMPLER_MIPMAP_MODE_NEAREST;
            }
        }


        VkBorderColor Translate(const Core::SamplerBorderColor color)
        {
            switch (color)
            {
            case Core::SamplerBorderColor::kTransparentBlack:
                return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
            case Core::SamplerBorderColor::kOpaqueBlack:
                return VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            case Core::SamplerBorderColor::kOpaqueWhite:
                return VK_BORDER_COLOR_INT_OPAQUE_WHITE;
            default:
                FE_DebugBreak();
                return VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            }
        }
    } // namespace


    VkSampler SamplerFactory::Create(const Device* device, const Core::SamplerState& samplerState)
    {
        FE_PROFILER_ZONE();

        VkSamplerCreateInfo samplerCI = {};
        samplerCI.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerCI.magFilter = Translate(samplerState.m_magFilter);
        samplerCI.minFilter = Translate(samplerState.m_minFilter);
        samplerCI.borderColor = Translate(samplerState.m_borderColor);
        samplerCI.unnormalizedCoordinates = VK_FALSE;
        samplerCI.mipmapMode = TranslateMipmapMode(samplerState.m_mipFilter);
        samplerCI.mipLodBias = static_cast<float>(samplerState.m_mipBias);
        samplerCI.minLod = static_cast<float>(samplerState.m_minLod);
        samplerCI.maxLod = static_cast<float>(samplerState.m_maxLod);

        const auto& adapterProperties = device->GetAdapterProperties();
        const float requestedAnisotropy = static_cast<float>(festd::to_underlying(samplerState.m_anisotropy));

        if (samplerState.m_anisotropy == Core::SamplerAnisotropy::kNone)
        {
            samplerCI.anisotropyEnable = false;
            samplerCI.maxAnisotropy = 0.0f;
        }
        else
        {
            samplerCI.anisotropyEnable = true;
            samplerCI.maxAnisotropy = Math::Min(requestedAnisotropy, adapterProperties.limits.maxSamplerAnisotropy);
        }

        samplerCI.addressModeU = Translate(samplerState.m_addressModeU);
        samplerCI.addressModeV = Translate(samplerState.m_addressModeV);
        samplerCI.addressModeW = Translate(samplerState.m_addressModeW);

        samplerCI.compareEnable = samplerState.m_compareEnable;
        samplerCI.compareOp = Translate(samplerState.m_compareOp);

        VkSampler sampler;
        VerifyVulkan(vkCreateSampler(device->GetNative(), &samplerCI, nullptr, &sampler));
        return sampler;
    }


    void SamplerFactory::Destroy(const Device* device, const VkSampler sampler)
    {
        FE_PROFILER_ZONE();

        if (sampler)
            vkDestroySampler(device->GetNative(), sampler, nullptr);
    }
} // namespace FE::Graphics::Vulkan
