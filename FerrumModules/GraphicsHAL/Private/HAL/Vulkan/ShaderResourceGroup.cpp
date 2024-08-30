#include <FeCore/Containers/SmallVector.h>
#include <HAL/ShaderReflection.h>
#include <HAL/Vulkan/Buffer.h>
#include <HAL/Vulkan/Image.h>
#include <HAL/Vulkan/ImageView.h>
#include <HAL/Vulkan/Sampler.h>
#include <HAL/Vulkan/ShaderResourceGroup.h>

namespace FE::Graphics::Vulkan
{
    inline static VkDescriptorType GetDescriptorType(HAL::ShaderResourceType type)
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
            FE_AssertMsg(false, "Invalid ShaderResourceType");
            return VK_DESCRIPTOR_TYPE_MAX_ENUM;
        }
    }


    ShaderResourceGroup::~ShaderResourceGroup()
    {
        if (m_LayoutHandle)
            m_pDescriptorAllocator->ReleaseDescriptorSetLayout(m_LayoutHandle);
    }


    HAL::ResultCode ShaderResourceGroup::Init(const HAL::ShaderResourceGroupDesc& desc)
    {
        festd::small_vector<VkDescriptorSetLayoutBinding> vkBindings;
        for (const HAL::ShaderReflection* pReflection : desc.ShadersReflection)
        {
            const festd::span bindings = pReflection->GetResourceBindings();
            for (uint32_t bindingIndex = 0; bindingIndex < bindings.size(); ++bindingIndex)
            {
                VkDescriptorSetLayoutBinding& vkBinding = vkBindings.emplace_back();
                vkBinding.binding = bindings[bindingIndex].Slot;
                vkBinding.descriptorCount = bindings[bindingIndex].Count;
                vkBinding.descriptorType = GetDescriptorType(bindings[bindingIndex].Type);
                vkBinding.stageFlags = VK_SHADER_STAGE_ALL;
            }

            eastl::copy(bindings.begin(), bindings.end(), eastl::back_inserter(m_ResourceBindings));
        }

        VkDescriptorSetLayoutCreateInfo layoutCreateInfo{};
        layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutCreateInfo.bindingCount = vkBindings.size();
        layoutCreateInfo.pBindings = vkBindings.data();
        m_LayoutHandle = m_pDescriptorAllocator->CreateDescriptorSetLayout(layoutCreateInfo);
        m_DescriptorSet = m_pDescriptorAllocator->AllocateDescriptorSet(m_LayoutHandle.Layout);

        eastl::sort(m_ResourceBindings.begin(),
                    m_ResourceBindings.end(),
                    [](const HAL::ShaderResourceBinding& lhs, const HAL::ShaderResourceBinding& rhs) {
                        return lhs.Slot < rhs.Slot;
                    });

        return HAL::ResultCode::Success;
    }


    void ShaderResourceGroup::Update(const HAL::ShaderResourceGroupData& data)
    {
        m_Data = data;

        for (const HAL::ShaderResourceEntry& entry : m_Data.GetEntries())
        {
            const HAL::ShaderResourceBinding* binding =
                eastl::lower_bound(m_ResourceBindings.begin(),
                                   m_ResourceBindings.end(),
                                   entry.Index,
                                   [](const HAL::ShaderResourceBinding& lhs, uint32_t rhs) {
                                       return lhs.Slot < rhs;
                                   });

            FE_Assert(binding != m_ResourceBindings.end() && binding && binding->Slot == entry.Index);

            VkWriteDescriptorSet writeDescriptorSet{};
            writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptorSet.descriptorType = GetDescriptorType(binding->Type);
            writeDescriptorSet.descriptorCount = binding->Count;
            writeDescriptorSet.dstArrayElement = 0;
            writeDescriptorSet.dstBinding = binding->Slot;
            writeDescriptorSet.dstSet = m_DescriptorSet;

            union
            {
                VkDescriptorBufferInfo bufferInfo;
                VkDescriptorImageInfo imageInfo;
            } info;

            if (const auto* pBuffer = fe_dynamic_cast<const Buffer*>(entry.pObject.Get()))
            {
                FE_Assert(HAL::IsBufferShaderResource(binding->Type));

                info.bufferInfo.buffer = NativeCast(pBuffer);
                info.bufferInfo.offset = 0;
                info.bufferInfo.range = pBuffer->GetDesc().Size;
                writeDescriptorSet.pBufferInfo = &info.bufferInfo;
            }
            if (const auto* pImageView = fe_dynamic_cast<const ImageView*>(entry.pObject.Get()))
            {
                FE_Assert(HAL::IsTextureShaderResource(binding->Type));

                info.imageInfo.imageView = NativeCast(pImageView);
                info.imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                info.imageInfo.sampler = VK_NULL_HANDLE;
                writeDescriptorSet.pImageInfo = &info.imageInfo;
            }
            if (const auto* pSampler = fe_dynamic_cast<const Sampler*>(entry.pObject.Get()))
            {
                FE_Assert(binding->Type == HAL::ShaderResourceType::Sampler);

                info.imageInfo.imageView = VK_NULL_HANDLE;
                info.imageInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                info.imageInfo.sampler = NativeCast(pSampler);
                writeDescriptorSet.pImageInfo = &info.imageInfo;
            }

            vkUpdateDescriptorSets(NativeCast(m_pDevice), 1, &writeDescriptorSet, 0, nullptr);
        }
    }
} // namespace FE::Graphics::Vulkan
