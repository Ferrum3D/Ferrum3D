#pragma once
#include <Graphics/Core/ShaderReflection.h>
#include <Graphics/Core/ShaderResourceGroup.h>
#include <Graphics/Core/Vulkan/Base/Config.h>
#include <Graphics/Core/Vulkan/DescriptorAllocator.h>

namespace FE::Graphics::Vulkan
{
    struct ShaderResourceGroup final : public Core::ShaderResourceGroup
    {
        FE_RTTI_Class(ShaderResourceGroup, "0C0CAFE4-1B82-423F-97C0-CC538F0F1FD6");

        ShaderResourceGroup(Core::Device* device, DescriptorAllocator* pDescriptorAllocator)
            : m_descriptorAllocator(pDescriptorAllocator)
        {
            m_device = device;
        }

        ~ShaderResourceGroup() override;

        Core::ResultCode Init(const Core::ShaderResourceGroupDesc& desc) override;
        void Update(const Core::ShaderResourceGroupData& data) override;

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

        DescriptorAllocator* m_descriptorAllocator = nullptr;
        festd::vector<Core::ShaderResourceBinding> m_resourceBindings;
    };

    FE_ENABLE_IMPL_CAST(ShaderResourceGroup);
} // namespace FE::Graphics::Vulkan
