#include <Core/Memory/FiberTempAllocator.h>
#include <Graphics/Core/Common/Texture.h>
#include <Graphics/Core/Vulkan/DescriptorManager.h>
#include <Graphics/Core/Vulkan/Device.h>
#include <Graphics/Core/Vulkan/ResourceInstance.h>

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


    DescriptorManager::DescriptorSetState::DescriptorSetState()
    {
        m_resourceDescriptorsToUpdate.resize(kResourceDescriptorCount, false);
        m_samplerDescriptorsToUpdate.resize(kSamplerDescriptorCount, false);
    }


    DescriptorManager::DescriptorManager(Core::Device* device)
    {
        FE_PROFILER_ZONE();

        m_device = ImplCast(device);
        m_fence = Fence::Create(m_device, 0);

        Memory::FiberTempAllocator temp;

        festd::pmr::vector<VkDescriptorPoolSize> sizes{ &temp };
        sizes.push_back({ VK_DESCRIPTOR_TYPE_MUTABLE_EXT, kResourceDescriptorCount * kMaxDescriptorSets });
        sizes.push_back({ VK_DESCRIPTOR_TYPE_SAMPLER, kSamplerDescriptorCount * kMaxDescriptorSets });

        VkDescriptorPoolCreateInfo poolCI = {};
        poolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolCI.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
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


    uint64_t DescriptorManager::GetDeviceAddress(const uint32_t descriptorIndex)
    {
        const Core::ResourceDescriptorInfo& descriptor = m_resourceDescriptors[descriptorIndex];
        FE_Assert(descriptor.m_resource != nullptr && descriptor.m_resource->GetType() == Core::ResourceType::kBuffer);
        FE_Assert(m_initializedResourceDescriptors.test(descriptorIndex));

        const Core::Buffer* buffer = Rtti::AssertCast<const Core::Buffer*>(descriptor.m_resource);

        VkBufferDeviceAddressInfo addressInfo{};
        addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        addressInfo.buffer = NativeCast(buffer);

        const VkDeviceAddress address = vkGetBufferDeviceAddress(NativeCast(m_device), &addressInfo);
        return address + descriptor.m_bufferSlice.m_offset;
    }


    void DescriptorManager::BeginFrame()
    {
        FE_PROFILER_ZONE();

        const uint64_t completedValue = m_fence->GetCompletedValue();

        for (uint32_t i = 0; i < m_descriptorSets.size(); ++i)
        {
            DescriptorSetState& set = m_descriptorSets[i];
            if (set.m_fenceValue <= completedValue)
            {
                ProcessCompletedDescriptorSet(i);
                if (m_currentSetIndex == kInvalidIndex)
                    m_currentSetIndex = i;
            }
        }

        ReleaseRetiredResourceDescriptors();

        if (m_currentSetIndex == kInvalidIndex)
        {
            FE_Assert(m_descriptorSets.size() < kMaxDescriptorSets, "Too many descriptor sets in flight");
            m_currentSetIndex = m_descriptorSets.size();
            DescriptorSetState& set = m_descriptorSets.emplace_back();
            set.m_set = AllocateDescriptorSet();
            Bit::Traverse(m_persistentResourceDescriptors.view() & m_initializedResourceDescriptors.view(),
                          [&set](const uint32_t index) {
                              set.m_resourceDescriptorsToUpdate.set(index);
                          });
            Bit::Traverse(m_persistentSamplerDescriptors.view() & m_initializedSamplerDescriptors.view(),
                          [&set](const uint32_t index) {
                              set.m_samplerDescriptorsToUpdate.set(index);
                          });
        }
    }


    Core::FenceSyncPoint DescriptorManager::CloseFrame()
    {
        FE_PROFILER_ZONE();
        FE_Assert(m_currentSetIndex != kInvalidIndex);
        DescriptorSetState& currentSet = m_descriptorSets[m_currentSetIndex];

        Bit::Traverse(m_resourceDescriptorsToUpdate.view() & m_persistentResourceDescriptors.view()
                          & m_initializedResourceDescriptors.view(),
                      [this](const uint32_t descriptorIndex) {
                          for (DescriptorSetState& set : m_descriptorSets)
                              set.m_resourceDescriptorsToUpdate.set(descriptorIndex);
                      });
        Bit::Traverse(m_samplerDescriptorsToUpdate.view() & m_persistentSamplerDescriptors.view()
                          & m_initializedSamplerDescriptors.view(),
                      [this](const uint32_t descriptorIndex) {
                          for (DescriptorSetState& set : m_descriptorSets)
                              set.m_samplerDescriptorsToUpdate.set(descriptorIndex);
                      });

        FlushPersistentResourceDescriptors(m_currentSetIndex);

        m_vkResourceDescriptors.reserve(m_resourceDescriptors.size());
        Bit::Traverse(m_resourceDescriptorsToUpdate.view() & m_transientResourceDescriptors.view()
                          & m_initializedResourceDescriptors.view(),
                      [this, &currentSet](const uint32_t descriptorIndex) {
                          AppendResourceDescriptorWrite(currentSet.m_set, descriptorIndex);
                      });

        if (!m_vkResourceDescriptors.empty())
        {
            vkUpdateDescriptorSets(NativeCast(m_device),
                                   m_vkResourceDescriptors.size(),
                                   m_vkResourceDescriptors.data(),
                                   0,
                                   nullptr);
        }

        m_vkResourceDescriptors.clear();
        m_linearAllocator.Clear();

        m_vkSamplerDescriptors.reserve(m_samplerDescriptors.size());
        const auto appendSamplerWrite = [this, &currentSet](const uint32_t descriptorIndex) {
            const Core::SamplerState descriptor = m_samplerDescriptors[descriptorIndex];
            const VkSampler sampler = m_device->GetSampler(descriptor);

            auto* imageInfo = Memory::New<VkDescriptorImageInfo>(&m_linearAllocator);
            imageInfo->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo->imageView = VK_NULL_HANDLE;
            imageInfo->sampler = sampler;
            m_vkSamplerDescriptors.push_back(
                CreateWrite(1, currentSet.m_set, VK_DESCRIPTOR_TYPE_SAMPLER, descriptorIndex, 1, imageInfo));
        };
        Bit::Traverse(m_samplerDescriptorsToUpdate.view() & m_transientSamplerDescriptors.view()
                          & m_initializedSamplerDescriptors.view(),
                      appendSamplerWrite);
        Bit::Traverse(currentSet.m_samplerDescriptorsToUpdate.view() & m_persistentSamplerDescriptors.view()
                          & m_initializedSamplerDescriptors.view(),
                      appendSamplerWrite);

        if (!m_vkSamplerDescriptors.empty())
        {
            FE_PROFILER_ZONE_NAMED("vkUpdateDescriptorSets");
            vkUpdateDescriptorSets(NativeCast(m_device),
                                   m_vkSamplerDescriptors.size(),
                                   m_vkSamplerDescriptors.data(),
                                   0,
                                   nullptr);
        }

        m_vkSamplerDescriptors.clear();
        m_linearAllocator.Clear();
        currentSet.m_samplerDescriptorsToUpdate.reset();

        m_resourceDescriptorsToUpdate.reset();
        m_samplerDescriptorsToUpdate.reset();
        ClearTransientDescriptors();

        currentSet.m_fenceValue = ++m_fenceValue;
        m_currentSetIndex = kInvalidIndex;
        return Core::FenceSyncPoint{ m_fence, m_fenceValue };
    }


    VkDescriptorSet DescriptorManager::AllocateDescriptorSet() const
    {
        FE_PROFILER_ZONE();

        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &m_descriptorSetLayout;

        VkDescriptorSet set;
        VerifyVk(vkAllocateDescriptorSets(NativeCast(m_device), &allocInfo, &set));
        return set;
    }


    void DescriptorManager::RetireResourceDescriptor(const uint32_t descriptorIndex, Core::Resource* resource)
    {
        if (m_descriptorSets.empty())
            return;

        RetiredResourceDescriptor& retired = m_retiredResourceDescriptors.emplace_back();
        retired.m_resource = resource;
        retired.m_descriptorIndex = descriptorIndex;
        retired.m_pendingSetMask = (1u << m_descriptorSets.size()) - 1;

        for (DescriptorSetState& set : m_descriptorSets)
            set.m_resourceDescriptorsToUpdate.set(descriptorIndex);
    }


    void DescriptorManager::AppendResourceDescriptorWrite(const VkDescriptorSet descriptorSet, const uint32_t descriptorIndex)
    {
        const Core::ResourceDescriptorInfo& descriptor = m_resourceDescriptors[descriptorIndex];

        switch (descriptor.m_resource->GetType())
        {
        default:
        case Core::ResourceType::kUnknown:
            FE_DebugBreak();
            break;

        case Core::ResourceType::kBuffer:
            {
                const Core::Buffer* buffer = Rtti::AssertCast<const Core::Buffer*>(descriptor.m_resource);
                const Core::BufferDesc bufferDesc = buffer->GetDesc();

                VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                if (bufferDesc.m_format != Core::Format::kUndefined)
                    descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;

                auto* bufferInfo = Memory::New<VkDescriptorBufferInfo>(&m_linearAllocator);
                bufferInfo->buffer = NativeCast(buffer);
                bufferInfo->offset = descriptor.m_bufferSlice.m_offset;
                bufferInfo->range = descriptor.m_bufferSlice.m_size;

                m_vkResourceDescriptors.push_back(CreateWrite(0, descriptorSet, descriptorType, descriptorIndex, 1, bufferInfo));
            }
            break;

        case Core::ResourceType::kTexture:
            {
                const Core::Texture* texture = Rtti::AssertCast<const Core::Texture*>(descriptor.m_resource);
                auto* instance = GetInstance(texture);

                VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                if (descriptor.m_descriptorType == Core::DescriptorType::kUAV)
                {
                    descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                    layout = VK_IMAGE_LAYOUT_GENERAL;
                }

                auto* imageInfo = Memory::New<VkDescriptorImageInfo>(&m_linearAllocator);
                imageInfo->imageLayout = layout;
                imageInfo->imageView = instance->GetSubresourceView(m_device, descriptor.m_textureSubresource);
                imageInfo->sampler = VK_NULL_HANDLE;

                m_vkResourceDescriptors.push_back(CreateWrite(0, descriptorSet, descriptorType, descriptorIndex, 1, imageInfo));
            }
            break;
        }
    }


    void DescriptorManager::FlushPersistentResourceDescriptors(const uint32_t setIndex)
    {
        DescriptorSetState& set = m_descriptorSets[setIndex];
        m_vkResourceDescriptors.reserve(m_resourceDescriptors.size());

        Bit::Traverse(set.m_resourceDescriptorsToUpdate.view() & m_persistentResourceDescriptors.view()
                          & m_initializedResourceDescriptors.view(),
                      [this, &set](const uint32_t descriptorIndex) {
                          AppendResourceDescriptorWrite(set.m_set, descriptorIndex);
                      });

        if (!m_vkResourceDescriptors.empty())
        {
            vkUpdateDescriptorSets(NativeCast(m_device),
                                   m_vkResourceDescriptors.size(),
                                   m_vkResourceDescriptors.data(),
                                   0,
                                   nullptr);
        }

        ReleaseRewrittenResourceDescriptors(setIndex);
        m_vkResourceDescriptors.clear();
        m_linearAllocator.Clear();
        set.m_resourceDescriptorsToUpdate.reset();
    }


    void DescriptorManager::ProcessCompletedDescriptorSet(const uint32_t setIndex)
    {
        DescriptorSetState& set = m_descriptorSets[setIndex];
        const uint32_t setMask = 1u << setIndex;
        const bool hasRetiredResources = eastl::any_of(m_retiredResourceDescriptors.begin(),
                                                       m_retiredResourceDescriptors.end(),
                                                       [setMask](const RetiredResourceDescriptor& retired) {
                                                           return (retired.m_pendingSetMask & setMask) != 0;
                                                       });
        if (hasRetiredResources)
        {
            VerifyVk(vkFreeDescriptorSets(NativeCast(m_device), m_descriptorPool, 1, &set.m_set));
            set.m_set = AllocateDescriptorSet();
            set.m_resourceDescriptorsToUpdate.reset();
            set.m_samplerDescriptorsToUpdate.reset();
            Bit::Traverse(m_persistentResourceDescriptors.view() & m_initializedResourceDescriptors.view(),
                          [&set](const uint32_t index) {
                              set.m_resourceDescriptorsToUpdate.set(index);
                          });
            Bit::Traverse(m_persistentSamplerDescriptors.view() & m_initializedSamplerDescriptors.view(),
                          [&set](const uint32_t index) {
                              set.m_samplerDescriptorsToUpdate.set(index);
                          });
        }

        FlushPersistentResourceDescriptors(setIndex);

        for (RetiredResourceDescriptor& retired : m_retiredResourceDescriptors)
        {
            if (!m_persistentResourceDescriptors.test(retired.m_descriptorIndex))
                retired.m_pendingSetMask &= ~setMask;
        }
    }


    void DescriptorManager::ReleaseRewrittenResourceDescriptors(const uint32_t setIndex)
    {
        const DescriptorSetState& set = m_descriptorSets[setIndex];
        const uint32_t setMask = 1u << setIndex;

        for (RetiredResourceDescriptor& retired : m_retiredResourceDescriptors)
        {
            if (set.m_resourceDescriptorsToUpdate.test(retired.m_descriptorIndex)
                && m_persistentResourceDescriptors.test(retired.m_descriptorIndex)
                && m_initializedResourceDescriptors.test(retired.m_descriptorIndex))
            {
                retired.m_pendingSetMask &= ~setMask;
            }
        }
    }


    void DescriptorManager::ReleaseRetiredResourceDescriptors()
    {
        m_retiredResourceDescriptors.erase(eastl::remove_if(m_retiredResourceDescriptors.begin(),
                                                            m_retiredResourceDescriptors.end(),
                                                            [](const RetiredResourceDescriptor& retired) {
                                                                return retired.m_pendingSetMask == 0;
                                                            }),
                                           m_retiredResourceDescriptors.end());
    }
} // namespace FE::Graphics::Vulkan
