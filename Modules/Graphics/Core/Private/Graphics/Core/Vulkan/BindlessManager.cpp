#include <FeCore/Memory/FiberTempAllocator.h>
#include <Graphics/Core/Vulkan/BindlessManager.h>
#include <Graphics/Core/Vulkan/Buffer.h>
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


        void CreateWrite(festd::pmr::vector<VkWriteDescriptorSet>& writes, uint32_t& binding, const VkDescriptorSet set,
                         const VkDescriptorType type, const uint32_t count, const VkDescriptorImageInfo* imageInfo)
        {
            const uint32_t currentBinding = binding++;
            if (count == 0)
                return;

            VkWriteDescriptorSet& write = writes.push_back();
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = set;
            write.dstBinding = currentBinding;
            write.descriptorType = type;
            write.descriptorCount = count;
            write.pImageInfo = imageInfo;
        }


        void CreateWrite(festd::pmr::vector<VkWriteDescriptorSet>& writes, uint32_t& binding, const VkDescriptorSet set,
                         const VkDescriptorType type, const uint32_t count, const VkDescriptorBufferInfo* bufferInfo)
        {
            const uint32_t currentBinding = binding++;
            if (count == 0)
                return;

            VkWriteDescriptorSet& write = writes.push_back();
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = set;
            write.dstBinding = currentBinding;
            write.descriptorType = type;
            write.descriptorCount = count;
            write.pBufferInfo = bufferInfo;
        }
    } // namespace


    BindlessManager::BindlessManager(Core::Device* device)
    {
        m_device = device;
        m_fence = Fence::Create(m_device);
        m_fence->Init(0);

        Memory::FiberTempAllocator temp;

        festd::pmr::vector<VkDescriptorPoolSize> sizes{ &temp };
        sizes.push_back({ VK_DESCRIPTOR_TYPE_SAMPLER, kSamplerCount * kMaxDescriptorSets });
        sizes.push_back({ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, kSampledImageCount * kMaxDescriptorSets });
        sizes.push_back({ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, kStorageImageCount * kMaxDescriptorSets });
        sizes.push_back({ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, kStorageBufferCount * kMaxDescriptorSets });

        VkDescriptorPoolCreateInfo poolCI = {};
        poolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolCI.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
        poolCI.maxSets = kMaxDescriptorSets;
        poolCI.poolSizeCount = sizes.size();
        poolCI.pPoolSizes = sizes.data();
        VerifyVulkan(vkCreateDescriptorPool(NativeCast(m_device), &poolCI, nullptr, &m_descriptorPool));

        festd::pmr::vector<VkDescriptorSetLayoutBinding> bindings{ &temp };
        bindings.push_back(CreateBinding(0, VK_DESCRIPTOR_TYPE_SAMPLER, kSamplerCount));
        bindings.push_back(CreateBinding(1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, kSampledImageCount));
        bindings.push_back(CreateBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, kStorageImageCount));
        bindings.push_back(CreateBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, kStorageBufferCount));

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

        VkDescriptorSetLayoutCreateInfo setLayoutCI = {};
        setLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        setLayoutCI.pNext = &flagsCI;
        setLayoutCI.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
        setLayoutCI.bindingCount = bindings.size();
        setLayoutCI.pBindings = bindings.data();
        VerifyVulkan(vkCreateDescriptorSetLayout(NativeCast(m_device), &setLayoutCI, nullptr, &m_descriptorSetLayout));
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

        uint32_t binding = 0;

        // clang-format off
        festd::pmr::vector<VkWriteDescriptorSet> writes{ &temp };
        CreateWrite(writes, binding, m_descriptorSet, VK_DESCRIPTOR_TYPE_SAMPLER, m_samplerDescriptors.size(), m_samplerDescriptors.data());
        CreateWrite(writes, binding, m_descriptorSet, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, m_sampledImageDescriptors.size(), m_sampledImageDescriptors.data());
        CreateWrite(writes, binding, m_descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, m_storageImageDescriptors.size(), m_storageImageDescriptors.data());
        CreateWrite(writes, binding, m_descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_storageBufferDescriptors.size(), m_storageBufferDescriptors.data());
        vkUpdateDescriptorSets(NativeCast(m_device), writes.size(), writes.data(), 0, nullptr);
        // clang-format on

        m_samplers.clear();
        m_samplerDescriptors.clear();
        m_sampledImageDescriptors.clear();
        m_sampledImageDescriptorMap.clear();
        m_storageImageDescriptors.clear();
        m_storageBufferDescriptors.clear();

        m_sampledImageDescriptorMap.clear();
        m_storageImageDescriptorMap.clear();
        m_storageBufferDescriptorMap.clear();

        RetiredSet retiredSet;
        retiredSet.m_set = m_descriptorSet;
        retiredSet.m_fenceValue = ++m_fenceValue;
        m_retiredSets.push_back(retiredSet);
        m_descriptorSet = VK_NULL_HANDLE;
        return Core::FenceSyncPoint{ m_fence, m_fenceValue };
    }


    uint32_t BindlessManager::RegisterSRV(Core::Texture* texture, const Core::ImageSubresource subresource)
    {
        const uint64_t key = static_cast<uint64_t>(texture->GetResourceID()) << 32 | festd::bit_cast<uint32_t>(subresource);

        const auto it = m_sampledImageDescriptorMap.find(key);
        if (it != m_sampledImageDescriptorMap.end())
        {
            const uint32_t descriptorIndex = it->second;
            FE_AssertDebug(m_sampledImageDescriptors[descriptorIndex].imageView
                           == ImplCast(texture)->GetSubresourceView(NativeCast(m_device), subresource));
            return descriptorIndex;
        }

        const uint32_t descriptorIndex = m_sampledImageDescriptors.size();
        m_sampledImageDescriptorMap[key] = descriptorIndex;

        VkDescriptorImageInfo& imageInfo = m_sampledImageDescriptors.push_back();
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = ImplCast(texture)->GetSubresourceView(NativeCast(m_device), subresource);
        imageInfo.sampler = VK_NULL_HANDLE;

        return descriptorIndex;
    }


    uint32_t BindlessManager::RegisterUAV(Core::Texture* texture, const Core::ImageSubresource subresource)
    {
        const uint64_t key = static_cast<uint64_t>(texture->GetResourceID()) << 32 | festd::bit_cast<uint32_t>(subresource);

        const auto it = m_storageImageDescriptorMap.find(key);
        if (it != m_storageImageDescriptorMap.end())
        {
            const uint32_t descriptorIndex = it->second;
            FE_AssertDebug(m_storageImageDescriptors[descriptorIndex].imageView
                           == ImplCast(texture)->GetSubresourceView(NativeCast(m_device), subresource));
            return descriptorIndex;
        }

        const uint32_t descriptorIndex = m_storageImageDescriptors.size();
        m_storageImageDescriptorMap[key] = descriptorIndex;

        VkDescriptorImageInfo& imageInfo = m_storageImageDescriptors.push_back();
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        imageInfo.imageView = ImplCast(texture)->GetSubresourceView(NativeCast(m_device), subresource);
        imageInfo.sampler = VK_NULL_HANDLE;

        return descriptorIndex;
    }


    uint32_t BindlessManager::RegisterSRV(Core::Buffer* buffer)
    {
        const uint64_t key = buffer->GetResourceID();

        const auto it = m_storageBufferDescriptorMap.find(key);
        if (it != m_storageBufferDescriptorMap.end())
        {
            const uint32_t descriptorIndex = it->second;
            FE_AssertDebug(m_storageBufferDescriptors[descriptorIndex].buffer == NativeCast(buffer));
            return descriptorIndex;
        }

        const uint32_t descriptorIndex = m_storageBufferDescriptors.size();
        m_storageBufferDescriptorMap[key] = descriptorIndex;

        VkDescriptorBufferInfo& bufferInfo = m_storageBufferDescriptors.push_back();
        bufferInfo.buffer = NativeCast(buffer);
        bufferInfo.offset = 0;
        bufferInfo.range = VK_WHOLE_SIZE;

        return descriptorIndex;
    }


    uint32_t BindlessManager::RegisterUAV(Core::Buffer* buffer)
    {
        // In Vulkan both SRV and UAV descriptors are mapped to storage buffers
        return RegisterSRV(buffer);
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
