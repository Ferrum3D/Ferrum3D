#include <Graphics/RHI/Vulkan/Device.h>
#include <Graphics/RHI/Vulkan/PipelineStates.h>
#include <Graphics/RHI/Vulkan/Sampler.h>

namespace FE::Graphics::Vulkan
{
    Sampler::Sampler(RHI::Device* device)
    {
        m_device = device;
    }


    RHI::ResultCode Sampler::Init(const RHI::SamplerDesc& desc)
    {
        m_desc = desc;

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

        switch (desc.m_anisotropy)
        {
        case RHI::SamplerAnisotropy::kNone:
            samplerCI.anisotropyEnable = false;
            samplerCI.maxAnisotropy = 0;
            break;

        case RHI::SamplerAnisotropy::kX16:
            samplerCI.anisotropyEnable = true;
            samplerCI.maxAnisotropy = 16;
            break;

        case RHI::SamplerAnisotropy::kMaxSupported:
            samplerCI.anisotropyEnable = true;
            samplerCI.maxAnisotropy = ImplCast(m_device)->GetAdapterProperties().limits.maxSamplerAnisotropy;
            break;
        }

        switch (desc.m_addressMode)
        {
        case RHI::SamplerAddressMode::kRepeat:
            samplerCI.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerCI.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            break;

        case RHI::SamplerAddressMode::kClamp:
            samplerCI.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerCI.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            break;
        }

        samplerCI.compareEnable = desc.m_compareEnable;
        samplerCI.compareOp = VKConvert(desc.m_compareOp);

        vkCreateSampler(NativeCast(m_device), &samplerCI, VK_NULL_HANDLE, &m_nativeSampler);
        return RHI::ResultCode::kSuccess;
    }


    const RHI::SamplerDesc& Sampler::GetDesc()
    {
        return m_desc;
    }


    Sampler::~Sampler()
    {
        if (m_nativeSampler)
            vkDestroySampler(NativeCast(m_device), m_nativeSampler, nullptr);
    }
} // namespace FE::Graphics::Vulkan
