#pragma once
#include <GPU/Common/VKConfig.h>
#include <GPU/Descriptors/IDescriptorHeap.h>

namespace FE::GPU
{
    class VKDevice;

    inline vk::DescriptorType GetDescriptorType(ShaderResourceType type)
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

    class VKDescriptorHeap : public Object<IDescriptorHeap>
    {
        vk::UniqueDescriptorPool m_NativePool;
        VKDevice* m_Device;

    public:
        FE_CLASS_RTTI(VKDescriptorHeap, "5AFA0C8B-35EE-4B53-9144-C3BD5A8AA51D");

        VKDescriptorHeap(VKDevice& dev, const DescriptorHeapDesc& desc);

        Shared<IDescriptorTable> AllocateDescriptorTable(const List<DescriptorDesc>& descriptors) override;

        vk::DescriptorPool& GetNativeDescriptorPool();
    };
} // namespace FE::GPU
