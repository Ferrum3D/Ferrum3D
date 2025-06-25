#include <FeCore/Memory/FiberTempAllocator.h>
#include <Graphics/Core/Vulkan/BindlessManager.h>
#include <Graphics/Core/Vulkan/Buffer.h>
#include <Graphics/Core/Vulkan/Device.h>
#include <Graphics/Core/Vulkan/RenderTarget.h>
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


    BindlessManager::BindlessManager(Core::Device* device)
        : m_writes(&m_frameAllocator)
    {
        m_device = device;
        m_fence = Fence::Create(m_device);
        m_fence->Init(0);

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
        VerifyVulkan(vkCreateDescriptorPool(NativeCast(m_device), &poolCI, nullptr, &m_descriptorPool));

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

        VkMutableDescriptorTypeListEXT typeList;
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
        VerifyVulkan(vkCreateDescriptorSetLayout(NativeCast(m_device), &setLayoutCI, nullptr, &m_descriptorSetLayout));
    }


    BindlessManager::~BindlessManager()
    {
        const VkDevice device = NativeCast(m_device);

        vkDestroyDescriptorSetLayout(device, m_descriptorSetLayout, nullptr);
        vkDestroyDescriptorPool(device, m_descriptorPool, nullptr);
    }


    void BindlessManager::BeginFrame()
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


    Core::FenceSyncPoint BindlessManager::CloseFrame()
    {
        Memory::FiberTempAllocator temp;

        if (!m_writes.empty())
        {
            festd::pmr::vector<VkWriteDescriptorSet> resourceWrites{ &temp };
            resourceWrites.reserve(m_writes.size());

            for (const VkWriteDescriptorSet& write : m_writes)
                resourceWrites.push_back(write);

            vkUpdateDescriptorSets(NativeCast(m_device), resourceWrites.size(), resourceWrites.data(), 0, nullptr);
        }

        if (!m_samplerDescriptors.empty())
        {
            const auto samplerWrite = CreateWrite(
                1, m_descriptorSet, VK_DESCRIPTOR_TYPE_SAMPLER, 0, m_samplerDescriptors.size(), m_samplerDescriptors.data());

            vkUpdateDescriptorSets(NativeCast(m_device), 1, &samplerWrite, 0, nullptr);
        }

        m_samplers.clear();
        m_samplerDescriptors.clear();
        m_sampledImageDescriptorMap.clear();
        m_storageImageDescriptorMap.clear();
        m_storageBufferDescriptorMap.clear();
        m_writes.reset_lose_memory();

        m_frameAllocator.Clear();

        RetiredSet retiredSet;
        retiredSet.m_set = m_descriptorSet;
        retiredSet.m_fenceValue = ++m_fenceValue;
        m_retiredSets.push_back(retiredSet);
        m_descriptorSet = VK_NULL_HANDLE;
        return Core::FenceSyncPoint{ m_fence, m_fenceValue };
    }


    uint32_t BindlessManager::RegisterSRV(const Core::Texture* texture, const Core::ImageSubresource subresource)
    {
        const uint64_t key = static_cast<uint64_t>(texture->GetResourceID()) << 32 | festd::bit_cast<uint32_t>(subresource);

        const auto it = m_sampledImageDescriptorMap.find(key);
        if (it != m_sampledImageDescriptorMap.end())
        {
            const uint32_t descriptorIndex = it->second;
            FE_AssertDebug(m_writes[descriptorIndex].pImageInfo->imageView
                           == ImplCast(texture)->GetSubresourceView(NativeCast(m_device), subresource));
            return descriptorIndex;
        }

        const uint32_t descriptorIndex = m_writes.size();
        m_sampledImageDescriptorMap[key] = descriptorIndex;

        auto* imageInfo = Memory::New<VkDescriptorImageInfo>(&m_frameAllocator);
        imageInfo->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo->imageView = ImplCast(texture)->GetSubresourceView(NativeCast(m_device), subresource);
        imageInfo->sampler = VK_NULL_HANDLE;

        m_writes.push_back(CreateWrite(0, m_descriptorSet, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, descriptorIndex, 1, imageInfo));

        return descriptorIndex;
    }


    uint32_t BindlessManager::RegisterSRV(const Core::RenderTarget* renderTarget, const Core::ImageSubresource subresource)
    {
        const uint64_t key = static_cast<uint64_t>(renderTarget->GetResourceID()) << 32 | festd::bit_cast<uint32_t>(subresource);

        const auto it = m_sampledImageDescriptorMap.find(key);
        if (it != m_sampledImageDescriptorMap.end())
        {
            const uint32_t descriptorIndex = it->second;
            FE_AssertDebug(m_writes[descriptorIndex].pImageInfo->imageView
                           == ImplCast(renderTarget)->GetSubresourceView(NativeCast(m_device), subresource));
            return descriptorIndex;
        }

        const uint32_t descriptorIndex = m_writes.size();
        m_sampledImageDescriptorMap[key] = descriptorIndex;

        auto* imageInfo = Memory::New<VkDescriptorImageInfo>(&m_frameAllocator);
        imageInfo->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo->imageView = ImplCast(renderTarget)->GetSubresourceView(NativeCast(m_device), subresource);
        imageInfo->sampler = VK_NULL_HANDLE;

        m_writes.push_back(CreateWrite(0, m_descriptorSet, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, descriptorIndex, 1, imageInfo));

        return descriptorIndex;
    }


    uint32_t BindlessManager::RegisterUAV(const Core::RenderTarget* renderTarget, const Core::ImageSubresource subresource)
    {
        const uint64_t key = static_cast<uint64_t>(renderTarget->GetResourceID()) << 32 | festd::bit_cast<uint32_t>(subresource);

        const auto it = m_storageImageDescriptorMap.find(key);
        if (it != m_storageImageDescriptorMap.end())
        {
            const uint32_t descriptorIndex = it->second;
            FE_AssertDebug(m_writes[descriptorIndex].pImageInfo->imageView
                           == ImplCast(renderTarget)->GetSubresourceView(NativeCast(m_device), subresource));
            return descriptorIndex;
        }

        const uint32_t descriptorIndex = m_writes.size();
        m_storageBufferDescriptorMap[key] = descriptorIndex;

        auto* imageInfo = Memory::New<VkDescriptorImageInfo>(&m_frameAllocator);
        imageInfo->imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        imageInfo->imageView = ImplCast(renderTarget)->GetSubresourceView(NativeCast(m_device), subresource);
        imageInfo->sampler = VK_NULL_HANDLE;

        m_writes.push_back(CreateWrite(0, m_descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, descriptorIndex, 1, imageInfo));

        return descriptorIndex;
    }


    uint32_t BindlessManager::RegisterSRV(const Core::Buffer* buffer, const uint32_t offset, const uint32_t size)
    {
        // TODO: take into account buffer size
        const uint64_t key = static_cast<uint64_t>(buffer->GetResourceID()) << 32 | offset;

        const auto it = m_storageBufferDescriptorMap.find(key);
        if (it != m_storageBufferDescriptorMap.end())
        {
            const uint32_t descriptorIndex = it->second;
            FE_AssertDebug(m_writes[descriptorIndex].pBufferInfo->buffer == NativeCast(buffer));
            return descriptorIndex;
        }

        const uint32_t descriptorIndex = m_writes.size();
        m_storageBufferDescriptorMap[key] = descriptorIndex;

        auto* bufferInfo = Memory::New<VkDescriptorBufferInfo>(&m_frameAllocator);
        bufferInfo->buffer = NativeCast(buffer);
        bufferInfo->offset = offset;
        bufferInfo->range = size;

        m_writes.push_back(CreateWrite(0, m_descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, descriptorIndex, 1, bufferInfo));

        return descriptorIndex;
    }


    uint32_t BindlessManager::RegisterUAV(const Core::Buffer* buffer, const uint32_t offset, const uint32_t size)
    {
        // In Vulkan both SRV and UAV descriptors are mapped to storage buffers
        return RegisterSRV(buffer, offset, size);
    }


    uint32_t BindlessManager::RegisterSampler(const Core::SamplerState sampler)
    {
        const VkSampler vkSampler = ImplCast(m_device)->GetSampler(sampler);

        for (uint32_t samplerIndex = 0; samplerIndex < m_samplers.size(); ++samplerIndex)
        {
            if (m_samplers[samplerIndex] == vkSampler)
                return samplerIndex;
        }

        const uint32_t descriptorIndex = m_samplers.size();
        m_samplers.push_back(vkSampler);

        VkDescriptorImageInfo& imageInfo = m_samplerDescriptors.push_back();
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = VK_NULL_HANDLE;
        imageInfo.sampler = vkSampler;

        return descriptorIndex;
    }


    VkDescriptorSet BindlessManager::AllocateDescriptorSet() const
    {
        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &m_descriptorSetLayout;

        VkDescriptorSet set;
        VerifyVulkan(vkAllocateDescriptorSets(NativeCast(m_device), &allocInfo, &set));
        return set;
    }
} // namespace FE::Graphics::Vulkan
