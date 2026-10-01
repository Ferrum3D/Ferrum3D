#include <Graphics/Core/DescriptorManager.h>

namespace FE::Graphics::Core
{
    ResourceDescriptorInfo::ResourceDescriptorInfo(const TextureView texture, const DescriptorType type)
        : m_resource(texture.m_resource)
        , m_descriptorType(type)
        , m_textureSubresource(texture.m_subresource)
    {
        FE_Assert(texture.IsValid());
        FE_Assert(type == DescriptorType::kSRV || type == DescriptorType::kUAV);
    }


    ResourceDescriptorInfo::ResourceDescriptorInfo(const BufferView buffer, const DescriptorType type)
        : m_resource(buffer.m_resource)
        , m_descriptorType(type)
        , m_bufferSlice(buffer.m_slice)
    {
        FE_Assert(buffer.IsValid());
        FE_Assert(type == DescriptorType::kSRV || type == DescriptorType::kUAV);
    }


    DescriptorManager::DescriptorManager()
    {
        m_freePersistentDescriptors.resize(kPersistentDescriptorCount, true);
    }


    TextureSRVDescriptor DescriptorManager::CreateSRV(const TextureView texture)
    {
        return { AllocatePersistentDescriptor({ texture, DescriptorType::kSRV }) };
    }


    TextureUAVDescriptor DescriptorManager::CreateUAV(const TextureView texture)
    {
        return { AllocatePersistentDescriptor({ texture, DescriptorType::kUAV }) };
    }


    BufferSRVDescriptor DescriptorManager::CreateSRV(const BufferView buffer)
    {
        return { AllocatePersistentDescriptor({ buffer, DescriptorType::kSRV }) };
    }


    BufferUAVDescriptor DescriptorManager::CreateUAV(const BufferView buffer)
    {
        return { AllocatePersistentDescriptor({ buffer, DescriptorType::kUAV }) };
    }


    void DescriptorManager::Update(const TextureSRVDescriptor descriptor, const TextureView texture)
    {
        UpdatePersistentDescriptor(descriptor.m_value, { texture, DescriptorType::kSRV });
    }


    void DescriptorManager::Update(const TextureUAVDescriptor descriptor, const TextureView texture)
    {
        UpdatePersistentDescriptor(descriptor.m_value, { texture, DescriptorType::kUAV });
    }


    void DescriptorManager::Update(const BufferSRVDescriptor descriptor, const BufferView buffer)
    {
        UpdatePersistentDescriptor(descriptor.m_value, { buffer, DescriptorType::kSRV });
    }


    void DescriptorManager::Update(const BufferUAVDescriptor descriptor, const BufferView buffer)
    {
        UpdatePersistentDescriptor(descriptor.m_value, { buffer, DescriptorType::kUAV });
    }


    uint32_t DescriptorManager::AllocatePersistentDescriptor(ResourceDescriptorInfo info)
    {
        FE_Assert(!m_frameActive, "Persistent descriptors are frozen for the current frame");
        CollectRetiredDescriptors();
        const uint32_t index = m_freePersistentDescriptors.find_first();
        FE_Assert(index != kInvalidIndex, "Persistent descriptor heap exhausted");
        m_freePersistentDescriptors.reset(index);
        if (index == m_persistentDescriptors.size())
            m_persistentDescriptors.push_back(std::move(info));
        else
            m_persistentDescriptors[index] = std::move(info);
        InvalidatePersistentDescriptor(index);
        return index;
    }


    void DescriptorManager::UpdatePersistentDescriptor(const uint32_t index, ResourceDescriptorInfo info)
    {
        FE_Assert(!m_frameActive, "Persistent descriptors are frozen for the current frame");
        const ResourceDescriptorInfo& previous = GetPersistentResourceInfo(index);
        FE_Assert(previous.m_resource->GetType() == info.m_resource->GetType());
        FE_Assert(previous.m_descriptorType == info.m_descriptorType);
        RetireDescriptor(index, false);
        m_persistentDescriptors[index] = std::move(info);
        InvalidatePersistentDescriptor(index);
    }


    void DescriptorManager::FreePersistentDescriptor(const uint32_t index)
    {
        FE_Assert(!m_frameActive, "Persistent descriptors are frozen for the current frame");
        FE_Assert(GetPersistentResourceInfo(index).m_resource);
        RetireDescriptor(index, true);
        m_persistentDescriptors[index] = {};
        InvalidatePersistentDescriptor(index);
    }


    const ResourceDescriptorInfo& DescriptorManager::GetPersistentResourceInfo(const uint32_t index) const
    {
        FE_Assert(index < m_persistentDescriptors.size());
        FE_Assert(m_persistentDescriptors[index].m_resource);
        return m_persistentDescriptors[index];
    }


    SamplerDescriptor DescriptorManager::GetSampler(const SamplerState sampler)
    {
        FE_Assert(!m_descriptorsPrepared, "Descriptors have already been published for this frame");
        const auto it = m_samplerMap.find(sampler);
        if (it != m_samplerMap.end())
            return { it->second };
        FE_Assert(m_samplers.size() < kSamplerDescriptorCount, "Sampler descriptor heap exhausted");
        const uint32_t index = m_samplers.size();
        m_samplers.push_back(sampler);
        m_samplerMap.emplace(sampler, index);
        return { index };
    }


    void DescriptorManager::RetireDescriptor(const uint32_t index, const bool releaseIndex)
    {
        RetiredDescriptor retired;
        retired.m_resource = m_persistentDescriptors[index].m_resource;
        retired.m_completion = m_lastFrameCompletion;
        retired.m_indexToRelease = releaseIndex ? index : kInvalidIndex;
        m_retiredDescriptors.push_back(std::move(retired));
    }


    void DescriptorManager::CollectRetiredDescriptors()
    {
        m_retiredDescriptors.erase(eastl::remove_if(m_retiredDescriptors.begin(),
                                                    m_retiredDescriptors.end(),
                                                    [this](const RetiredDescriptor& retired) {
                                                        if (retired.m_completion.m_fence && !retired.m_completion.IsReady())
                                                            return false;
                                                        if (retired.m_indexToRelease != kInvalidIndex)
                                                            m_freePersistentDescriptors.set(retired.m_indexToRelease);
                                                        return true;
                                                    }),
                                   m_retiredDescriptors.end());
    }


    void DescriptorManager::BeginFrame(const FenceSyncPoint& completion)
    {
        FE_Assert(!m_frameActive);
        FE_Assert(completion.m_fence);
        if (m_lastFrameCompletion.m_fence)
        {
            FE_Assert(completion.m_fence == m_lastFrameCompletion.m_fence);
            FE_Assert(completion.m_value > m_lastFrameCompletion.m_value);
        }
        CollectRetiredDescriptors();
        BeginFrameInternal(completion);
        m_lastFrameCompletion = completion;
        m_frameActive = true;
        m_descriptorsPrepared = false;
    }


    void DescriptorManager::PrepareDescriptors(const festd::span<const ResourceDescriptorInfo> descriptors)
    {
        FE_Assert(m_frameActive && !m_descriptorsPrepared);
        FE_Assert(descriptors.size() <= kTransientDescriptorCount);
        PrepareDescriptorsInternal(descriptors);
        m_descriptorsPrepared = true;
    }


    void DescriptorManager::EndFrame()
    {
        FE_Assert(m_frameActive && m_descriptorsPrepared);
        m_frameActive = false;
        m_descriptorsPrepared = false;
    }
} // namespace FE::Graphics::Core
