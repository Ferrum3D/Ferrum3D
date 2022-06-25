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
        vk::SamplerCreateInfo samplerCI   = {};
        samplerCI.magFilter               = vk::Filter::eLinear;
        samplerCI.minFilter               = vk::Filter::eLinear;
        samplerCI.borderColor             = vk::BorderColor::eIntOpaqueBlack;
        samplerCI.unnormalizedCoordinates = VK_FALSE;
        samplerCI.mipmapMode              = vk::SamplerMipmapMode::eLinear;
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
            samplerCI.maxAnisotropy    = static_cast<VKAdapter&>(m_Device->GetAdapter()).Prop.limits.maxSamplerAnisotropy;
            break;
        }

        switch (desc.AddressMode)
        {
        case SamplerAddressMode::Repeat:
            samplerCI.addressModeU = vk::SamplerAddressMode::eRepeat;
            samplerCI.addressModeV = vk::SamplerAddressMode::eRepeat;
            samplerCI.addressModeW = vk::SamplerAddressMode::eRepeat;
            break;
        case SamplerAddressMode::Clamp:
            samplerCI.addressModeU = vk::SamplerAddressMode::eClampToEdge;
            samplerCI.addressModeV = vk::SamplerAddressMode::eClampToEdge;
            samplerCI.addressModeW = vk::SamplerAddressMode::eClampToEdge;
            break;
        }

        samplerCI.compareEnable = desc.CompareEnable;
        samplerCI.compareOp     = VKConvert(desc.CompareOp);

        Sampler = m_Device->GetNativeDevice().createSamplerUnique(samplerCI);
    }

    const SamplerDesc& VKSampler::GetDesc()
    {
        return m_Desc;
    }
} // namespace FE::Osmium
