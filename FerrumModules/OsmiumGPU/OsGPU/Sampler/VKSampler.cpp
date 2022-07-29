#include <OsGPU/Adapter/VKAdapter.h>
#include <OsGPU/Device/VKDevice.h>
#include <OsGPU/Pipeline/VKPipelineStates.h>
#include <OsGPU/Sampler/VKSampler.h>

namespace FE::Osmium
{
    VKSampler::VKSampler(VKDevice& dev, const SamplerDesc& desc)
        : m_Device(&dev)
        , m_Desc(desc)
    {
        VkSamplerCreateInfo samplerCI     = {};
        samplerCI.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerCI.magFilter               = VK_FILTER_LINEAR;
        samplerCI.minFilter               = VK_FILTER_LINEAR;
        samplerCI.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerCI.unnormalizedCoordinates = VK_FALSE;
        samplerCI.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerCI.mipLodBias              = 0.0f;
        samplerCI.minLod                  = 0.0f;
        samplerCI.maxLod                  = 100000.0f;

        switch (desc.Anisotropy)
        {
        case SamplerAnisotropy::X16:
            samplerCI.anisotropyEnable = true;
            samplerCI.maxAnisotropy    = 16;
            break;
        case SamplerAnisotropy::None:
            samplerCI.anisotropyEnable = false;
            samplerCI.maxAnisotropy    = 0;
            break;
        case SamplerAnisotropy::MaxSupported:
            samplerCI.anisotropyEnable = true;
            samplerCI.maxAnisotropy = fe_assert_cast<VKAdapter*>(&m_Device->GetAdapter())->Properties.limits.maxSamplerAnisotropy;
            break;
        }

        switch (desc.AddressMode)
        {
        case SamplerAddressMode::Repeat:
            samplerCI.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerCI.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            break;
        case SamplerAddressMode::Clamp:
            samplerCI.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerCI.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            break;
        }

        samplerCI.compareEnable = desc.CompareEnable;
        samplerCI.compareOp     = VKConvert(desc.CompareOp);

        vkCreateSampler(m_Device->GetNativeDevice(), &samplerCI, VK_NULL_HANDLE, &Sampler);
    }

    const SamplerDesc& VKSampler::GetDesc()
    {
        return m_Desc;
    }

    FE_VK_OBJECT_DELETER(Sampler);

    VKSampler::~VKSampler()
    {
        FE_DELETE_VK_OBJECT(Sampler, Sampler);
    }
} // namespace FE::Osmium
