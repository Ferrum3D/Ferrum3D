#include <HAL/Vulkan/Device.h>
#include <HAL/Vulkan/PipelineStates.h>
#include <HAL/Vulkan/Sampler.h>

namespace FE::Graphics::Vulkan
{
    Sampler::Sampler(HAL::Device* pDevice)
    {
        m_pDevice = pDevice;
    }


    HAL::ResultCode Sampler::Init(const HAL::SamplerDesc& desc)
    {
        m_Desc = desc;

        VkSamplerCreateInfo samplerCI = {};
        samplerCI.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerCI.magFilter = VK_FILTER_LINEAR;
        samplerCI.minFilter = VK_FILTER_LINEAR;
        samplerCI.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerCI.unnormalizedCoordinates = VK_FALSE;
        samplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerCI.mipLodBias = 0.0f;
        samplerCI.minLod = 0.0f;
        samplerCI.maxLod = 100000.0f;

        switch (desc.Anisotropy)
        {
        case HAL::SamplerAnisotropy::None:
            samplerCI.anisotropyEnable = false;
            samplerCI.maxAnisotropy = 0;
            break;
        case HAL::SamplerAnisotropy::X16:
            samplerCI.anisotropyEnable = true;
            samplerCI.maxAnisotropy = 16;
            break;
        case HAL::SamplerAnisotropy::MaxSupported:
            samplerCI.anisotropyEnable = true;
            samplerCI.maxAnisotropy = ImplCast(m_pDevice)->GetAdapterProperties().limits.maxSamplerAnisotropy;
            break;
        }

        switch (desc.AddressMode)
        {
        case HAL::SamplerAddressMode::Repeat:
            samplerCI.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerCI.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            break;
        case HAL::SamplerAddressMode::Clamp:
            samplerCI.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerCI.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            break;
        }

        samplerCI.compareEnable = desc.CompareEnable;
        samplerCI.compareOp = VKConvert(desc.CompareOp);

        vkCreateSampler(NativeCast(m_pDevice), &samplerCI, VK_NULL_HANDLE, &m_NativeSampler);
        return HAL::ResultCode::Success;
    }


    const HAL::SamplerDesc& Sampler::GetDesc()
    {
        return m_Desc;
    }


    Sampler::~Sampler()
    {
        if (m_NativeSampler)
            vkDestroySampler(NativeCast(m_pDevice), m_NativeSampler, nullptr);
    }
} // namespace FE::Graphics::Vulkan
