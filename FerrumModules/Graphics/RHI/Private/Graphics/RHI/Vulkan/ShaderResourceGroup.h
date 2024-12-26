#pragma once
#include <Graphics/RHI/ShaderReflection.h>
#include <Graphics/RHI/ShaderResourceGroup.h>
#include <Graphics/RHI/Vulkan/Common/Config.h>
#include <Graphics/RHI/Vulkan/DescriptorAllocator.h>

namespace FE::Graphics::Vulkan
{
    struct ShaderResourceGroup final : public RHI::ShaderResourceGroup
    {
        FE_RTTI_Class(ShaderResourceGroup, "0C0CAFE4-1B82-423F-97C0-CC538F0F1FD6");

        ShaderResourceGroup(RHI::Device* device, DescriptorAllocator* pDescriptorAllocator)
            : m_pDescriptorAllocator(pDescriptorAllocator)
        {
            m_device = device;
        }

        ~ShaderResourceGroup() override;

        RHI::ResultCode Init(const RHI::ShaderResourceGroupDesc& desc) override;
        void Update(const RHI::ShaderResourceGroupData& data) override;

        VkDescriptorSet GetNativeSet() const
        {
            return m_descriptorSet;
        }

        VkDescriptorSetLayout GetNativeSetLayout() const
        {
            return m_layoutHandle.m_layout;
        }

    private:
        VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;
        DescriptorSetLayoutHandle m_layoutHandle;

        DescriptorAllocator* m_pDescriptorAllocator = nullptr;
        festd::vector<RHI::ShaderResourceBinding> m_resourceBindings;
    };

    FE_ENABLE_IMPL_CAST(ShaderResourceGroup);
} // namespace FE::Graphics::Vulkan
