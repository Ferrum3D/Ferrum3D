#pragma once
#include <OsGPU/Common/VKConfig.h>
#include <OsGPU/Descriptors/IDescriptorHeap.h>

namespace FE::Osmium
{
    class VKDevice;

    inline VkDescriptorType GetDescriptorType(ShaderResourceType type)
    {
        switch (type)
        {
        case ShaderResourceType::ConstantBuffer:
            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        case ShaderResourceType::TextureSRV:
            return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        case ShaderResourceType::TextureUAV:
            return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        case ShaderResourceType::BufferSRV:
            return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
        case ShaderResourceType::BufferUAV:
            return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        case ShaderResourceType::Sampler:
            return VK_DESCRIPTOR_TYPE_SAMPLER;
        case ShaderResourceType::InputAttachment:
            return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        default:
            FE_UNREACHABLE("Invalid ShaderResourceType");
            return VK_DESCRIPTOR_TYPE_MAX_ENUM;
        }
    }

    class VKDescriptorHeap : public Object<IDescriptorHeap>
    {
        VkDescriptorPool m_NativePool;
        VKDevice* m_Device;

    public:
        FE_CLASS_RTTI(VKDescriptorHeap, "5AFA0C8B-35EE-4B53-9144-C3BD5A8AA51D");

        VKDescriptorHeap(VKDevice& dev, const DescriptorHeapDesc& desc);
        ~VKDescriptorHeap() override;

        Shared<IDescriptorTable> AllocateDescriptorTable(const ArraySlice<DescriptorDesc>& descriptors) override;

        void Reset() override;

        VkDescriptorPool GetNativeDescriptorPool();
    };
} // namespace FE::Osmium
