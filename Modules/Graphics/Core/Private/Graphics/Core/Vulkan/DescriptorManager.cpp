#include <Core/Memory/FiberTempAllocator.h>
#include <Graphics/Core/Common/Texture.h>
#include <Graphics/Core/Vulkan/DescriptorManager.h>
#include <Graphics/Core/Vulkan/Device.h>
#include <Graphics/Core/Vulkan/ResourceInstance.h>

namespace FE::Graphics::Vulkan
{
    namespace
    {
        constexpr uint32_t kStaticSamplerBindingBase = 2;


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
        FE_PROFILER_ZONE();

        m_device = ImplCast(device);

        Memory::FiberTempAllocator temp;

        // Keep the order in sync with Shaders/Base/StaticSamplers.hlsli.
        const VkSampler staticSamplers[] = {
            m_device->GetSampler(Core::SamplerState::kPointWrap),
            m_device->GetSampler(Core::SamplerState::kPointMirror),
            m_device->GetSampler(Core::SamplerState::kPointClamp),
            m_device->GetSampler(Core::SamplerState::kPointBorderTransparentBlack),
            m_device->GetSampler(Core::SamplerState::kLinearWrap),
            m_device->GetSampler(Core::SamplerState::kLinearMirror),
            m_device->GetSampler(Core::SamplerState::kLinearClamp),
            m_device->GetSampler(Core::SamplerState::kLinearBorderTransparentBlack),
        };
        const uint32_t staticSamplerCount = festd::size(staticSamplers);

        festd::pmr::vector<VkDescriptorPoolSize> sizes{ &temp };
        sizes.push_back({ VK_DESCRIPTOR_TYPE_MUTABLE_EXT, kResourceDescriptorCount * kMaxInFlightFrames });
        sizes.push_back({ VK_DESCRIPTOR_TYPE_SAMPLER, (kSamplerDescriptorCount + staticSamplerCount) * kMaxInFlightFrames });

        VkDescriptorPoolCreateInfo poolCI = {};
        poolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        // Use the bindless descriptor limits; sets themselves are frozen while in flight.
        poolCI.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
        poolCI.maxSets = kMaxInFlightFrames;
        poolCI.poolSizeCount = sizes.size();
        poolCI.pPoolSizes = sizes.data();
        VerifyVk(vkCreateDescriptorPool(NativeCast(m_device), &poolCI, nullptr, &m_descriptorPool));

        festd::pmr::vector<VkDescriptorSetLayoutBinding> bindings{ &temp };
        bindings.push_back(CreateBinding(0, VK_DESCRIPTOR_TYPE_MUTABLE_EXT, kResourceDescriptorCount));
        bindings.push_back(CreateBinding(1, VK_DESCRIPTOR_TYPE_SAMPLER, kSamplerDescriptorCount));

        for (uint32_t samplerIndex = 0; samplerIndex < staticSamplerCount; ++samplerIndex)
        {
            VkDescriptorSetLayoutBinding& binding = bindings.push_back();
            binding = CreateBinding(kStaticSamplerBindingBase + samplerIndex, VK_DESCRIPTOR_TYPE_SAMPLER, 1);
            binding.pImmutableSamplers = &staticSamplers[samplerIndex];
        }

        festd::pmr::vector<VkDescriptorBindingFlags> descriptorBindingFlags{ &temp };

        for (uint32_t i = 0; i < bindings.size(); ++i)
        {
            const bool isBindlessBinding = bindings[i].binding < kStaticSamplerBindingBase;
            descriptorBindingFlags.push_back(
                isBindlessBinding ? VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT : 0);
        }

        VkDescriptorSetLayoutBindingFlagsCreateInfo flagsCI = {};
        flagsCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
        flagsCI.bindingCount = descriptorBindingFlags.size();
        flagsCI.pBindingFlags = descriptorBindingFlags.data();

        constexpr VkDescriptorType mutableDescriptorTypes[] = { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                                                                VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                                                                VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
                                                                VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
                                                                VK_DESCRIPTOR_TYPE_STORAGE_BUFFER };

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

        for (DescriptorSetState& set : m_descriptorSets)
        {
            VkDescriptorSetAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = m_descriptorPool;
            allocInfo.descriptorSetCount = 1;
            allocInfo.pSetLayouts = &m_descriptorSetLayout;
            VerifyVk(vkAllocateDescriptorSets(NativeCast(m_device), &allocInfo, &set.m_set));
            set.m_dirtyDescriptors.resize(kPersistentDescriptorCount, false);
        }
    }


    DescriptorManager::~DescriptorManager()
    {
        for (DescriptorSetState& set : m_descriptorSets)
        {
            if (set.m_completion.m_fence)
                set.m_completion.Wait();
            set.m_transientResources.clear();
        }
        vkDestroyDescriptorPool(NativeCast(m_device), m_descriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(NativeCast(m_device), m_descriptorSetLayout, nullptr);
    }


    void DescriptorManager::BeginFrameInternal(const Core::FenceSyncPoint& completion)
    {
        m_currentSetIndex = static_cast<uint32_t>(completion.m_value % kMaxInFlightFrames);
        DescriptorSetState& set = m_descriptorSets[m_currentSetIndex];
        if (set.m_completion.m_fence)
            set.m_completion.Wait();
        set.m_transientResources.clear();
        set.m_completion = completion;
    }


    void DescriptorManager::InvalidatePersistentDescriptor(const uint32_t descriptorIndex)
    {
        for (DescriptorSetState& set : m_descriptorSets)
            set.m_dirtyDescriptors.set(descriptorIndex);
    }


    void DescriptorManager::PrepareDescriptorsInternal(const festd::span<const Core::ResourceDescriptorInfo> descriptors)
    {
        FE_PROFILER_ZONE();
        DescriptorSetState& set = m_descriptorSets[m_currentSetIndex];
        Bit::Traverse(set.m_dirtyDescriptors.view(), [this](const uint32_t index) {
            const Core::ResourceDescriptorInfo& descriptor = m_persistentDescriptors[index];
            if (descriptor.m_resource)
                AppendResourceDescriptorWrite(index, descriptor);
        });
        set.m_dirtyDescriptors.reset();

        for (uint32_t index = 0; index < descriptors.size(); ++index)
        {
            const Core::ResourceDescriptorInfo& descriptor = descriptors[index];
            if (!descriptor.m_resource)
                continue;
            AppendResourceDescriptorWrite(kPersistentDescriptorCount + index, descriptor);
            set.m_transientResources.push_back(descriptor.m_resource);
        }

        for (uint32_t index = set.m_samplerCount; index < m_samplers.size(); ++index)
        {
            auto* imageInfo = Memory::New<VkDescriptorImageInfo>(&m_linearAllocator);
            *imageInfo = {};
            imageInfo->sampler = m_device->GetSampler(m_samplers[index]);
            m_writes.push_back(CreateWrite(1, set.m_set, VK_DESCRIPTOR_TYPE_SAMPLER, index, 1, imageInfo));
        }
        set.m_samplerCount = m_samplers.size();

        if (!m_writes.empty())
            vkUpdateDescriptorSets(NativeCast(m_device), m_writes.size(), m_writes.data(), 0, nullptr);
        m_writes.clear();
        m_linearAllocator.Clear();
    }


    void DescriptorManager::AppendResourceDescriptorWrite(const uint32_t index, const Core::ResourceDescriptorInfo& descriptor)
    {
        const VkDescriptorSet set = GetDescriptorSet();
        const bool isUAV = descriptor.m_descriptorType == Core::DescriptorType::kUAV;
        if (descriptor.m_resource->GetType() == Core::ResourceType::kBuffer)
        {
            const auto* buffer = Rtti::AssertCast<const Core::Buffer*>(descriptor.m_resource.Get());
            if (buffer->GetDesc().m_format != Core::Format::kUndefined)
            {
                auto* view = Memory::New<VkBufferView>(&m_linearAllocator);
                *view = GetInstance(buffer)->GetSliceView(m_device, descriptor.m_bufferSlice);
                VkWriteDescriptorSet write = {};
                write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                write.dstSet = set;
                write.dstBinding = 0;
                write.dstArrayElement = index;
                write.descriptorCount = 1;
                write.descriptorType = isUAV ? VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER : VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
                write.pTexelBufferView = view;
                m_writes.push_back(write);
                return;
            }

            auto* bufferInfo = Memory::New<VkDescriptorBufferInfo>(&m_linearAllocator);
            bufferInfo->buffer = NativeCast(buffer);
            bufferInfo->offset = descriptor.m_bufferSlice.m_offset;
            bufferInfo->range = descriptor.m_bufferSlice.m_size;
            m_writes.push_back(CreateWrite(0, set, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, index, 1, bufferInfo));
            return;
        }

        const auto* texture = Rtti::AssertCast<const Core::Texture*>(descriptor.m_resource.Get());
        auto* imageInfo = Memory::New<VkDescriptorImageInfo>(&m_linearAllocator);
        imageInfo->imageLayout = isUAV ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo->imageView = GetInstance(texture)->GetSubresourceView(m_device, descriptor.m_textureSubresource);
        imageInfo->sampler = VK_NULL_HANDLE;
        const VkDescriptorType type = isUAV ? VK_DESCRIPTOR_TYPE_STORAGE_IMAGE : VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        m_writes.push_back(CreateWrite(0, set, type, index, 1, imageInfo));
    }
} // namespace FE::Graphics::Vulkan
