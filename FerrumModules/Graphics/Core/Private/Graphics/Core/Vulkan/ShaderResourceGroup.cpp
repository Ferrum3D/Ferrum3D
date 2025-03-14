#include <Graphics/Core/ShaderReflection.h>
#include <Graphics/Core/Vulkan/Buffer.h>
#include <Graphics/Core/Vulkan/Image.h>
#include <Graphics/Core/Vulkan/ShaderResourceGroup.h>
#include <festd/vector.h>

namespace FE::Graphics::Vulkan
{
    namespace
    {
        VkDescriptorType GetDescriptorType(const Core::ShaderResourceType type)
        {
            switch (type)
            {
            case Core::ShaderResourceType::kConstantBuffer:
                return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            case Core::ShaderResourceType::kTextureSRV:
                return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            case Core::ShaderResourceType::kTextureUAV:
                return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            case Core::ShaderResourceType::kBufferSRV:
                return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
            case Core::ShaderResourceType::kBufferUAV:
                return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            case Core::ShaderResourceType::kSampler:
                return VK_DESCRIPTOR_TYPE_SAMPLER;
            case Core::ShaderResourceType::kInputAttachment:
                return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
            case Core::ShaderResourceType::kNone:
            default:
                FE_AssertMsg(false, "Invalid ShaderResourceType");
                return VK_DESCRIPTOR_TYPE_MAX_ENUM;
            }
        }
    } // namespace


    ShaderResourceGroup::~ShaderResourceGroup()
    {
        if (m_layoutHandle)
            m_descriptorAllocator->ReleaseDescriptorSetLayout(m_layoutHandle);
    }


    Core::ResultCode ShaderResourceGroup::Init(const Core::ShaderResourceGroupDesc& desc)
    {
        FE_PROFILER_ZONE();

        festd::small_vector<VkDescriptorSetLayoutBinding> vkBindings;
        for (const Core::ShaderReflection* pReflection : desc.m_shadersReflection)
        {
            const festd::span bindings = pReflection->GetResourceBindings();
            for (const Core::ShaderResourceBinding& binding : bindings)
            {
                VkDescriptorSetLayoutBinding& vkBinding = vkBindings.emplace_back();
                vkBinding.binding = binding.m_slot;
                vkBinding.descriptorCount = binding.m_count;
                vkBinding.descriptorType = GetDescriptorType(binding.m_type);
                vkBinding.stageFlags = VK_SHADER_STAGE_ALL;
            }

            eastl::copy(bindings.begin(), bindings.end(), eastl::back_inserter(m_resourceBindings));
        }

        VkDescriptorSetLayoutCreateInfo layoutCreateInfo{};
        layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutCreateInfo.bindingCount = vkBindings.size();
        layoutCreateInfo.pBindings = vkBindings.data();
        m_layoutHandle = m_descriptorAllocator->CreateDescriptorSetLayout(layoutCreateInfo);
        m_descriptorSet = m_descriptorAllocator->AllocateDescriptorSet(m_layoutHandle.m_layout);

        festd::sort(m_resourceBindings, [](const Core::ShaderResourceBinding& lhs, const Core::ShaderResourceBinding& rhs) {
            return lhs.m_slot < rhs.m_slot;
        });

        return Core::ResultCode::kSuccess;
    }


    void ShaderResourceGroup::Update(const Core::ShaderResourceGroupData& data)
    {
        FE_PROFILER_ZONE();

        m_data = data;

        for (const Core::ShaderResourceEntry& entry : m_data.GetEntries())
        {
            const Core::ShaderResourceBinding* binding =
                eastl::lower_bound(m_resourceBindings.begin(),
                                   m_resourceBindings.end(),
                                   entry.m_index,
                                   [](const Core::ShaderResourceBinding& lhs, const uint32_t rhs) {
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

            if (entry.m_type == Core::ShaderResourceEntry::Type::kImage)
            {
                auto* buffer = fe_assert_cast<const Buffer*>(entry.m_resource.Get());
                FE_Assert(Core::IsBufferShaderResource(binding->m_type));

                const Core::BufferDesc& bufferDesc = buffer->GetDesc();
                FE_Assert(entry.m_bufferSubresource.m_offset + entry.m_bufferSubresource.m_size <= bufferDesc.m_size);

                info.bufferInfo.buffer = NativeCast(buffer);
                info.bufferInfo.offset = entry.m_bufferSubresource.m_offset;
                info.bufferInfo.range = entry.m_bufferSubresource.m_size;
                writeDescriptorSet.pBufferInfo = &info.bufferInfo;
            }
            else if (entry.m_type == Core::ShaderResourceEntry::Type::kBuffer)
            {
                auto* image = fe_assert_cast<Image*>(entry.m_resource.Get());
                FE_Assert(Core::IsTextureShaderResource(binding->m_type));

                info.imageInfo.imageView = ImplCast(image)->GetSubresourceView(entry.m_imageSubresource);
                info.imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                info.imageInfo.sampler = VK_NULL_HANDLE;
                writeDescriptorSet.pImageInfo = &info.imageInfo;
            }
            else if (entry.m_type == Core::ShaderResourceEntry::Type::kSampler)
            {
                FE_Assert(binding->m_type == Core::ShaderResourceType::kSampler);

                info.imageInfo.imageView = VK_NULL_HANDLE;
                info.imageInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                info.imageInfo.sampler = ImplCast(m_device)->GetSampler(entry.m_sampler);
                writeDescriptorSet.pImageInfo = &info.imageInfo;
            }
            else
            {
                FE_DebugBreak();
            }

            vkUpdateDescriptorSets(NativeCast(m_device), 1, &writeDescriptorSet, 0, nullptr);
        }
    }
} // namespace FE::Graphics::Vulkan
