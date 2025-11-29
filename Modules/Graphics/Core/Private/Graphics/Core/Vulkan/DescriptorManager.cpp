#include <FeCore/Memory/FiberTempAllocator.h>
#include <Graphics/Core/Vulkan/Buffer.h>
#include <Graphics/Core/Vulkan/DescriptorManager.h>
#include <Graphics/Core/Vulkan/Device.h>
#include <Graphics/Core/Vulkan/Texture.h>

namespace FE::Graphics::Vulkan
{
    namespace
    {
        VkDescriptorSetLayoutBinding CreateBinding(const uint32_t binding, const VkDescriptorType type, const uint32_t count)
        {
            VkDescriptorSetLayoutBinding bindingInfo;
            bindingInfo.binding = binding;
            bindingInfo.descriptorType = type;
            bindingInfo.descriptorCount = count;
            bindingInfo.stageFlags = VK_SHADER_STAGE_ALL;
            bindingInfo.pImmutableSamplers = nullptr;
            return bindingInfo;
        }


        VkWriteDescriptorSet CreateWrite(const uint32_t binding, const VkDescriptorSet set, const VkDescriptorType type,
                                         const uint32_t arrayElement, const uint32_t count,
                                         const VkDescriptorImageInfo* imageInfo)
        {
            VkWriteDescriptorSet write = {};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = set;
            write.dstBinding = binding;
            write.dstArrayElement = arrayElement;
            write.descriptorType = type;
            write.descriptorCount = count;
            write.pImageInfo = imageInfo;
            return write;
        }


        VkWriteDescriptorSet CreateWrite(const uint32_t binding, const VkDescriptorSet set, const VkDescriptorType type,
                                         const uint32_t arrayElement, const uint32_t count,
                                         const VkDescriptorBufferInfo* bufferInfo)
        {
            VkWriteDescriptorSet write = {};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = set;
            write.dstBinding = binding;
            write.dstArrayElement = arrayElement;
            write.descriptorType = type;
            write.descriptorCount = count;
            write.pBufferInfo = bufferInfo;
            return write;
        }
    } // namespace


    DescriptorManager::DescriptorManager(Core::Device* device)
    {
        m_device = ImplCast(device);
        m_fence = Fence::Create(m_device, 0);

        Memory::FiberTempAllocator temp;

        festd::pmr::vector<VkDescriptorPoolSize> sizes{ &temp };
        sizes.push_back({ VK_DESCRIPTOR_TYPE_MUTABLE_EXT, kResourceDescriptorCount * kMaxDescriptorSets });
        sizes.push_back({ VK_DESCRIPTOR_TYPE_SAMPLER, kSamplerDescriptorCount * kMaxDescriptorSets });

        VkDescriptorPoolCreateInfo poolCI = {};
        poolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolCI.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
        poolCI.maxSets = kMaxDescriptorSets;
        poolCI.poolSizeCount = sizes.size();
        poolCI.pPoolSizes = sizes.data();
        VerifyVk(vkCreateDescriptorPool(NativeCast(m_device), &poolCI, nullptr, &m_descriptorPool));

        festd::pmr::vector<VkDescriptorSetLayoutBinding> bindings{ &temp };
        bindings.push_back(CreateBinding(0, VK_DESCRIPTOR_TYPE_MUTABLE_EXT, kResourceDescriptorCount));
        bindings.push_back(CreateBinding(1, VK_DESCRIPTOR_TYPE_SAMPLER, kSamplerDescriptorCount));

        festd::pmr::vector<VkDescriptorBindingFlags> descriptorBindingFlags{ &temp };

        for (uint32_t i = 0; i < bindings.size(); ++i)
        {
            descriptorBindingFlags.push_back(VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT
                                             | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT);
        }

        VkDescriptorSetLayoutBindingFlagsCreateInfo flagsCI = {};
        flagsCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
        flagsCI.bindingCount = descriptorBindingFlags.size();
        flagsCI.pBindingFlags = descriptorBindingFlags.data();

        constexpr VkDescriptorType mutableDescriptorTypes[] = {
            VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,        VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
            VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
        };

        VkMutableDescriptorTypeListEXT typeList = {};
        typeList.descriptorTypeCount = sizeof(mutableDescriptorTypes) / sizeof(VkDescriptorType);
        typeList.pDescriptorTypes = mutableDescriptorTypes;

        VkMutableDescriptorTypeCreateInfoEXT mutableTypeInfo = {};
        mutableTypeInfo.sType = VK_STRUCTURE_TYPE_MUTABLE_DESCRIPTOR_TYPE_CREATE_INFO_EXT;
        mutableTypeInfo.pNext = &flagsCI;
        mutableTypeInfo.mutableDescriptorTypeListCount = 1;
        mutableTypeInfo.pMutableDescriptorTypeLists = &typeList;

        VkDescriptorSetLayoutCreateInfo setLayoutCI = {};
        setLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        setLayoutCI.pNext = &mutableTypeInfo;
        setLayoutCI.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
        setLayoutCI.bindingCount = bindings.size();
        setLayoutCI.pBindings = bindings.data();
        VerifyVk(vkCreateDescriptorSetLayout(NativeCast(m_device), &setLayoutCI, nullptr, &m_descriptorSetLayout));
    }


    DescriptorManager::~DescriptorManager()
    {
        const VkDevice device = NativeCast(m_device);

        vkDestroyDescriptorSetLayout(device, m_descriptorSetLayout, nullptr);
        vkDestroyDescriptorPool(device, m_descriptorPool, nullptr);
    }


    void DescriptorManager::BeginFrame()
    {
        const uint64_t completedValue = m_fence->GetCompletedValue();

        for (uint32_t i = 0; i < m_retiredSets.size(); ++i)
        {
            const RetiredSet& set = m_retiredSets[i];
            if (set.m_fenceValue <= completedValue)
            {
                m_descriptorSet = set.m_set;
                m_retiredSets.erase_unsorted(m_retiredSets.begin() + i);
                break;
            }
        }

        if (m_descriptorSet == VK_NULL_HANDLE)
        {
            m_descriptorSet = AllocateDescriptorSet();
        }
    }


    Core::FenceSyncPoint DescriptorManager::CloseFrame()
    {
        m_vkResourceDescriptors.reserve(m_resourceDescriptors.size());

        Bit::Traverse(m_committedResourceDescriptors.view(), [this](const uint32_t descriptorIndex) {
            const ResourceDescriptorInfo& descriptor = m_resourceDescriptors[descriptorIndex];

            switch (descriptor.m_resource->GetType())
            {
            default:
            case Core::ResourceType::kUnknown:
                FE_DebugBreak();
                break;

            case Core::ResourceType::kBuffer:
                {
                    const Core::Buffer* buffer = RTTI::AssertCast<const Core::Buffer*>(descriptor.m_resource);
                    const Core::BufferDesc bufferDesc = buffer->GetDesc();

                    VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                    if (bufferDesc.m_format != Core::Format::kUndefined)
                        descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;

                    auto* bufferInfo = Memory::New<VkDescriptorBufferInfo>(&m_linearAllocator);
                    bufferInfo->buffer = NativeCast(buffer);
                    bufferInfo->offset = descriptor.m_bufferSubresource.m_offset;
                    bufferInfo->range = descriptor.m_bufferSubresource.m_size;

                    m_vkResourceDescriptors.push_back(
                        CreateWrite(0, m_descriptorSet, descriptorType, descriptorIndex, 1, bufferInfo));
                }
                break;

            case Core::ResourceType::kTexture:
                {
                    const Core::Texture* texture = RTTI::AssertCast<const Core::Texture*>(descriptor.m_resource);

                    VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                    VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    if (descriptor.m_descriptorType == Core::DescriptorType::kUAV)
                    {
                        descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                        layout = VK_IMAGE_LAYOUT_GENERAL;
                    }

                    auto* imageInfo = Memory::New<VkDescriptorImageInfo>(&m_linearAllocator);
                    imageInfo->imageLayout = layout;
                    imageInfo->imageView = ImplCast(texture)->GetSubresourceView(descriptor.m_textureSubresource);
                    imageInfo->sampler = VK_NULL_HANDLE;

                    m_vkResourceDescriptors.push_back(
                        CreateWrite(0, m_descriptorSet, descriptorType, descriptorIndex, 1, imageInfo));
                }
                break;
            }
        });

        if (!m_vkResourceDescriptors.empty())
        {
            vkUpdateDescriptorSets(
                NativeCast(m_device), m_vkResourceDescriptors.size(), m_vkResourceDescriptors.data(), 0, nullptr);
        }

        m_vkResourceDescriptors.clear();
        m_linearAllocator.Clear();

        m_vkSamplerDescriptors.reserve(m_samplerDescriptors.size());

        Bit::Traverse(m_committedSamplerDescriptors.view(), [this](const uint32_t descriptorIndex) {
            const Core::SamplerState descriptor = m_samplerDescriptors[descriptorIndex];
            const VkSampler sampler = m_device->GetSampler(descriptor);

            VkDescriptorImageInfo& imageInfo = m_vkSamplerDescriptors.push_back();
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = VK_NULL_HANDLE;
            imageInfo.sampler = sampler;
        });

        if (!m_vkSamplerDescriptors.empty())
        {
            const auto samplerWrite = CreateWrite(
                1, m_descriptorSet, VK_DESCRIPTOR_TYPE_SAMPLER, 0, m_vkSamplerDescriptors.size(), m_vkSamplerDescriptors.data());

            vkUpdateDescriptorSets(NativeCast(m_device), 1, &samplerWrite, 0, nullptr);
        }

        m_vkSamplerDescriptors.clear();

        Clear();

        RetiredSet retiredSet;
        retiredSet.m_set = m_descriptorSet;
        retiredSet.m_fenceValue = ++m_fenceValue;
        m_retiredSets.push_back(retiredSet);
        m_descriptorSet = VK_NULL_HANDLE;
        return Core::FenceSyncPoint{ m_fence, m_fenceValue };
    }


    VkDescriptorSet DescriptorManager::AllocateDescriptorSet() const
    {
        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &m_descriptorSetLayout;

        VkDescriptorSet set;
        VerifyVk(vkAllocateDescriptorSets(NativeCast(m_device), &allocInfo, &set));
        return set;
    }
} // namespace FE::Graphics::Vulkan
