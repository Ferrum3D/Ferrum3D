#include <FeGPU/Descriptors/VKDescriptorHeap.h>
#include <FeGPU/Device/VKDevice.h>

namespace FE::GPU
{
    vk::DescriptorType GetDescriptorType(ShaderResourceType type)
    {
        switch (type)
        {
        case ShaderResourceType::ConstantBuffer:
            return vk::DescriptorType::eUniformBuffer;
        case ShaderResourceType::TextureSRV:
            return vk::DescriptorType::eSampledImage;
        case ShaderResourceType::TextureUAV:
            return vk::DescriptorType::eStorageImage;
        case ShaderResourceType::BufferSRV:
            return vk::DescriptorType::eUniformTexelBuffer;
        case ShaderResourceType::BufferUAV:
            return vk::DescriptorType::eStorageTexelBuffer;
        case ShaderResourceType::Sampler:
            return vk::DescriptorType::eSampler;
        case ShaderResourceType::InputAttachment:
            return vk::DescriptorType::eInputAttachment;
        default:
            FE_UNREACHABLE("Invalid ShaderResourceType");
            return static_cast<vk::DescriptorType>(-1);
        }
    }

    VKDescriptorHeap::VKDescriptorHeap(VKDevice& dev, const DescriptorHeapDesc& desc)
        : m_Device(&dev)
    {
        Vector<vk::DescriptorPoolSize> sizes;
        sizes.reserve(desc.Sizes.size());
        for (auto& size : desc.Sizes)
        {
            auto& nativeSize           = sizes.emplace_back();
            nativeSize.descriptorCount = size.DescriptorCount;
            nativeSize.type            = GetDescriptorType(size.ResourceType);
        }

        vk::DescriptorPoolCreateInfo poolCI{};
        poolCI.maxSets       = desc.MaxSets;
        poolCI.pPoolSizes    = sizes.data();
        poolCI.poolSizeCount = static_cast<UInt32>(sizes.size());

        m_NativePool = m_Device->GetNativeDevice().createDescriptorPoolUnique(poolCI);
    }

    vk::DescriptorPool& VKDescriptorHeap::GetNativeDescriptorPool()
    {
        return m_NativePool.get();
    }
} // namespace FE::GPU
