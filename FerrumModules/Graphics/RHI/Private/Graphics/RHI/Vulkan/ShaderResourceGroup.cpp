#include <FeCore/Containers/SmallVector.h>
#include <Graphics/RHI/ShaderReflection.h>
#include <Graphics/RHI/Vulkan/Buffer.h>
#include <Graphics/RHI/Vulkan/Image.h>
#include <Graphics/RHI/Vulkan/ImageView.h>
#include <Graphics/RHI/Vulkan/Sampler.h>
#include <Graphics/RHI/Vulkan/ShaderResourceGroup.h>

namespace FE::Graphics::Vulkan
{
    inline static VkDescriptorType GetDescriptorType(RHI::ShaderResourceType type)
    {
        switch (type)
        {
        case RHI::ShaderResourceType::kConstantBuffer:
            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        case RHI::ShaderResourceType::kTextureSRV:
            return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        case RHI::ShaderResourceType::kTextureUAV:
            return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        case RHI::ShaderResourceType::kBufferSRV:
            return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
        case RHI::ShaderResourceType::kBufferUAV:
            return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        case RHI::ShaderResourceType::kSampler:
            return VK_DESCRIPTOR_TYPE_SAMPLER;
        case RHI::ShaderResourceType::kInputAttachment:
            return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        default:
            FE_AssertMsg(false, "Invalid ShaderResourceType");
            return VK_DESCRIPTOR_TYPE_MAX_ENUM;
        }
    }


    ShaderResourceGroup::~ShaderResourceGroup()
    {
        if (m_layoutHandle)
            m_pDescriptorAllocator->ReleaseDescriptorSetLayout(m_layoutHandle);
    }


    RHI::ResultCode ShaderResourceGroup::Init(const RHI::ShaderResourceGroupDesc& desc)
    {
        festd::small_vector<VkDescriptorSetLayoutBinding> vkBindings;
        for (const RHI::ShaderReflection* pReflection : desc.m_shadersReflection)
        {
            const festd::span bindings = pReflection->GetResourceBindings();
            for (uint32_t bindingIndex = 0; bindingIndex < bindings.size(); ++bindingIndex)
            {
                VkDescriptorSetLayoutBinding& vkBinding = vkBindings.emplace_back();
                vkBinding.binding = bindings[bindingIndex].m_slot;
                vkBinding.descriptorCount = bindings[bindingIndex].m_count;
                vkBinding.descriptorType = GetDescriptorType(bindings[bindingIndex].m_type);
                vkBinding.stageFlags = VK_SHADER_STAGE_ALL;
            }

            eastl::copy(bindings.begin(), bindings.end(), eastl::back_inserter(m_resourceBindings));
        }

        VkDescriptorSetLayoutCreateInfo layoutCreateInfo{};
        layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutCreateInfo.bindingCount = vkBindings.size();
        layoutCreateInfo.pBindings = vkBindings.data();
        m_layoutHandle = m_pDescriptorAllocator->CreateDescriptorSetLayout(layoutCreateInfo);
        m_descriptorSet = m_pDescriptorAllocator->AllocateDescriptorSet(m_layoutHandle.Layout);

        eastl::sort(m_resourceBindings.begin(),
                    m_resourceBindings.end(),
                    [](const RHI::ShaderResourceBinding& lhs, const RHI::ShaderResourceBinding& rhs) {
                        return lhs.m_slot < rhs.m_slot;
                    });

        return RHI::ResultCode::kSuccess;
    }


    void ShaderResourceGroup::Update(const RHI::ShaderResourceGroupData& data)
    {
        m_data = data;

        for (const RHI::ShaderResourceEntry& entry : m_data.GetEntries())
        {
            const RHI::ShaderResourceBinding* binding =
                eastl::lower_bound(m_resourceBindings.begin(),
                                   m_resourceBindings.end(),
                                   entry.m_index,
                                   [](const RHI::ShaderResourceBinding& lhs, uint32_t rhs) {
                                       return lhs.m_slot < rhs;
                                   });

            FE_Assert(binding != m_resourceBindings.end() && binding && binding->m_slot == entry.m_index);

            VkWriteDescriptorSet writeDescriptorSet{};
            writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptorSet.descriptorType = GetDescriptorType(binding->m_type);
            writeDescriptorSet.descriptorCount = binding->m_count;
            writeDescriptorSet.dstArrayElement = 0;
            writeDescriptorSet.dstBinding = binding->m_slot;
            writeDescriptorSet.dstSet = m_descriptorSet;

            union
            {
                VkDescriptorBufferInfo bufferInfo;
                VkDescriptorImageInfo imageInfo;
            } info;

            if (const auto* pBuffer = fe_dynamic_cast<const Buffer*>(entry.m_object.Get()))
            {
                FE_Assert(RHI::IsBufferShaderResource(binding->m_type));

                info.bufferInfo.buffer = NativeCast(pBuffer);
                info.bufferInfo.offset = 0;
                info.bufferInfo.range = pBuffer->GetDesc().m_size;
                writeDescriptorSet.pBufferInfo = &info.bufferInfo;
            }

            if (const auto* pImageView = fe_dynamic_cast<const ImageView*>(entry.m_object.Get()))
            {
                FE_Assert(RHI::IsTextureShaderResource(binding->m_type));

                info.imageInfo.imageView = NativeCast(pImageView);
                info.imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                info.imageInfo.sampler = VK_NULL_HANDLE;
                writeDescriptorSet.pImageInfo = &info.imageInfo;
            }

            if (const auto* pSampler = fe_dynamic_cast<const Sampler*>(entry.m_object.Get()))
            {
                FE_Assert(binding->m_type == RHI::ShaderResourceType::kSampler);

                info.imageInfo.imageView = VK_NULL_HANDLE;
                info.imageInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                info.imageInfo.sampler = NativeCast(pSampler);
                writeDescriptorSet.pImageInfo = &info.imageInfo;
            }

            vkUpdateDescriptorSets(NativeCast(m_device), 1, &writeDescriptorSet, 0, nullptr);
        }
    }
} // namespace FE::Graphics::Vulkan
