#pragma once
#include <HAL/ShaderReflection.h>
#include <HAL/ShaderResourceGroup.h>
#include <HAL/Vulkan/Common/Config.h>
#include <HAL/Vulkan/DescriptorAllocator.h>

namespace FE::Graphics::Vulkan
{
    class ShaderResourceGroup final : public HAL::ShaderResourceGroup
    {
        VkDescriptorSet m_DescriptorSet = VK_NULL_HANDLE;
        DescriptorSetLayoutHandle m_LayoutHandle;

        DescriptorAllocator* m_pDescriptorAllocator = nullptr;
        festd::vector<HAL::ShaderResourceBinding> m_ResourceBindings;

    public:
        FE_RTTI_Class(ShaderResourceGroup, "0C0CAFE4-1B82-423F-97C0-CC538F0F1FD6");

        inline ShaderResourceGroup(HAL::Device* pDevice, DescriptorAllocator* pDescriptorAllocator)
            : m_pDescriptorAllocator(pDescriptorAllocator)
        {
            m_pDevice = pDevice;
        }

        ~ShaderResourceGroup() override;

        HAL::ResultCode Init(const HAL::ShaderResourceGroupDesc& desc) override;
        void Update(const HAL::ShaderResourceGroupData& data) override;

        inline VkDescriptorSet GetNativeSet() const
        {
            return m_DescriptorSet;
        }

        inline VkDescriptorSetLayout GetNativeSetLayout() const
        {
            return m_LayoutHandle.Layout;
        }
    };

    FE_ENABLE_IMPL_CAST(ShaderResourceGroup);
} // namespace FE::Graphics::Vulkan
