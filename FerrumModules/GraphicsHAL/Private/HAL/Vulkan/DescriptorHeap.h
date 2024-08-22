#pragma once
#include <HAL/DescriptorHeap.h>
#include <HAL/Vulkan/Common/Config.h>

namespace FE::Graphics::Vulkan
{
    inline VkDescriptorType GetDescriptorType(HAL::ShaderResourceType type)
    {
        switch (type)
        {
        case HAL::ShaderResourceType::ConstantBuffer:
            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        case HAL::ShaderResourceType::TextureSRV:
            return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        case HAL::ShaderResourceType::TextureUAV:
            return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        case HAL::ShaderResourceType::BufferSRV:
            return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
        case HAL::ShaderResourceType::BufferUAV:
            return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        case HAL::ShaderResourceType::Sampler:
            return VK_DESCRIPTOR_TYPE_SAMPLER;
        case HAL::ShaderResourceType::InputAttachment:
            return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        default:
            FE_UNREACHABLE("Invalid ShaderResourceType");
            return VK_DESCRIPTOR_TYPE_MAX_ENUM;
        }
    }


    class DescriptorAllocator;

    class DescriptorHeap : public HAL::DescriptorHeap
    {
        VkDescriptorPool m_NativePool = VK_NULL_HANDLE;
        Rc<DescriptorAllocator> m_DescriptorAllocator;

    public:
        FE_RTTI_Class(DescriptorHeap, "5AFA0C8B-35EE-4B53-9144-C3BD5A8AA51D");

        DescriptorHeap(HAL::Device* pDevice, DescriptorAllocator* pDescriptorAllocator);
        ~DescriptorHeap() override;

        HAL::ResultCode Init(const HAL::DescriptorHeapDesc& desc) override;

        Rc<HAL::DescriptorTable> AllocateDescriptorTable(festd::span<const HAL::DescriptorDesc> descriptors) override;

        void Reset() override;

        inline VkDescriptorPool GetNativeDescriptorPool() const
        {
            return m_NativePool;
        }
    };

    FE_ENABLE_IMPL_CAST(DescriptorHeap);
} // namespace FE::Graphics::Vulkan
