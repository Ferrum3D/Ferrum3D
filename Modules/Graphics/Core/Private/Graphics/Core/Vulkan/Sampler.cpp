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
    } // namespace


    VkSampler SamplerFactory::Create(const Device* device, const Core::SamplerState& samplerState)
    {
        FE_PROFILER_ZONE();

        VkSamplerCreateInfo samplerCI = {};
        samplerCI.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerCI.magFilter = VK_FILTER_LINEAR;
        samplerCI.minFilter = VK_FILTER_LINEAR;
        samplerCI.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerCI.unnormalizedCoordinates = VK_FALSE;
        samplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
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
