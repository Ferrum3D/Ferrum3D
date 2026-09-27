#include <Graphics/Core/DescriptorManager.h>

namespace FE::Graphics::Core
{
    ResourceDescriptorInfo::ResourceDescriptorInfo()
        : m_textureSubresource(TextureSubresource::kInvalid)
    {
    }


    ResourceDescriptorInfo::ResourceDescriptorInfo(const TextureView texture)
    {
        m_resource = texture.m_resource;
        m_textureSubresource = texture.m_subresource;
    }


    ResourceDescriptorInfo::ResourceDescriptorInfo(const BufferView buffer)
    {
        m_resource = buffer.m_resource;
        m_bufferSlice = buffer.m_slice;
    }


    DescriptorManager::DescriptorManager()
    {
        m_freeResourceDescriptors.resize(kResourceDescriptorCount, true);
        m_transientResourceDescriptors.resize(kResourceDescriptorCount, false);
        m_persistentResourceDescriptors.resize(kResourceDescriptorCount, false);
        m_initializedResourceDescriptors.resize(kResourceDescriptorCount, false);
        m_resourceDescriptorsToUpdate.resize(kResourceDescriptorCount, false);

        m_freeSamplerDescriptors.resize(kSamplerDescriptorCount, true);
        m_transientSamplerDescriptors.resize(kSamplerDescriptorCount, false);
        m_persistentSamplerDescriptors.resize(kSamplerDescriptorCount, false);
        m_initializedSamplerDescriptors.resize(kSamplerDescriptorCount, false);
        m_samplerDescriptorsToUpdate.resize(kSamplerDescriptorCount, false);
    }


    void DescriptorManager::ClearTransientDescriptors()
    {
        Bit::Traverse(m_transientResourceDescriptors.view(), [this](const uint32_t descriptorIndex) {
            const ResourceDescriptorInfo& descriptor = m_resourceDescriptors[descriptorIndex];
            if (descriptor.m_resource->GetType() == ResourceType::kTexture)
            {
                const TextureKey key{ descriptor.m_resource->GetResourceID(),
                                      descriptor.m_textureSubresource,
                                      DescriptorLifetime::kTransient };
                m_textureDescriptorMap.erase(key);
            }
            else
            {
                const BufferKey key{ descriptor.m_resource->GetResourceID(),
                                     descriptor.m_bufferSlice,
                                     DescriptorLifetime::kTransient };
                m_bufferDescriptorMap.erase(key);
            }

            RetireTransientResourceDescriptor(descriptorIndex, descriptor.m_resource);
            m_resourceDescriptors[descriptorIndex] = ResourceDescriptorInfo{};
            m_initializedResourceDescriptors.reset(descriptorIndex);
            m_resourceDescriptorsToUpdate.reset(descriptorIndex);
        });
        m_transientResourceDescriptors.reset();

        Bit::Traverse(m_transientSamplerDescriptors.view(), [this](const uint32_t descriptorIndex) {
            m_samplerDescriptorMap.erase({ m_samplerDescriptors[descriptorIndex], DescriptorLifetime::kTransient });
            m_samplerDescriptors[descriptorIndex] = SamplerState::kPointWrap;
            RetireTransientSamplerDescriptor(descriptorIndex);
            m_initializedSamplerDescriptors.reset(descriptorIndex);
            m_samplerDescriptorsToUpdate.reset(descriptorIndex);
        });
        m_transientSamplerDescriptors.reset();
    }


    void DescriptorManager::ReleaseResourceDescriptorIndex(const uint32_t descriptorIndex)
    {
        FE_Assert(!m_freeResourceDescriptors.test(descriptorIndex));
        m_freeResourceDescriptors.set(descriptorIndex);
    }


    void DescriptorManager::ReleaseSamplerDescriptorIndex(const uint32_t descriptorIndex)
    {
        FE_Assert(!m_freeSamplerDescriptors.test(descriptorIndex));
        m_freeSamplerDescriptors.set(descriptorIndex);
    }


    uint32_t DescriptorManager::ReserveDescriptor(const TextureView texture, const DescriptorLifetime lifetime)
    {
        TextureKey key;
        key.m_resourceID = texture.m_resource->GetResourceID();
        key.m_subresource = texture.m_subresource;
        key.m_lifetime = lifetime;

        const auto it = m_textureDescriptorMap.find(key);
        if (it != m_textureDescriptorMap.end())
            return it->second;

        const uint32_t descriptorIndex = m_freeResourceDescriptors.find_first();
        FE_Assert(descriptorIndex != kInvalidIndex, "Resource descriptor heap exhausted");
        m_freeResourceDescriptors.reset(descriptorIndex);

        switch (lifetime)
        {
        case DescriptorLifetime::kTransient:
            m_transientResourceDescriptors.set(descriptorIndex);
            break;
        case DescriptorLifetime::kPersistent:
            m_persistentResourceDescriptors.set(descriptorIndex);
            break;
        }

        if (descriptorIndex == m_resourceDescriptors.size())
            m_resourceDescriptors.push_back(ResourceDescriptorInfo{ texture });
        else
            m_resourceDescriptors[descriptorIndex] = ResourceDescriptorInfo{ texture };

        m_textureDescriptorMap[key] = descriptorIndex;
        return descriptorIndex;
    }


    uint32_t DescriptorManager::ReserveDescriptor(const BufferView buffer, const DescriptorLifetime lifetime)
    {
        BufferKey key;
        key.m_resourceID = buffer.m_resource->GetResourceID();
        key.m_subresource = buffer.m_slice;
        key.m_lifetime = lifetime;

        const auto it = m_bufferDescriptorMap.find(key);
        if (it != m_bufferDescriptorMap.end())
            return it->second;

        const uint32_t descriptorIndex = m_freeResourceDescriptors.find_first();
        FE_Assert(descriptorIndex != kInvalidIndex, "Resource descriptor heap exhausted");
        m_freeResourceDescriptors.reset(descriptorIndex);
        (lifetime == DescriptorLifetime::kPersistent ? m_persistentResourceDescriptors : m_transientResourceDescriptors)
            .set(descriptorIndex);

        if (descriptorIndex == m_resourceDescriptors.size())
            m_resourceDescriptors.push_back(ResourceDescriptorInfo{ buffer });
        else
            m_resourceDescriptors[descriptorIndex] = ResourceDescriptorInfo{ buffer };
        m_bufferDescriptorMap[key] = descriptorIndex;
        return descriptorIndex;
    }


    uint32_t DescriptorManager::ReserveDescriptor(const SamplerState samplerState, const DescriptorLifetime lifetime)
    {
        const SamplerKey key{ samplerState, lifetime };
        const auto it = m_samplerDescriptorMap.find(key);
        if (it != m_samplerDescriptorMap.end())
            return it->second;

        const uint32_t descriptorIndex = m_freeSamplerDescriptors.find_first();
        FE_Assert(descriptorIndex != kInvalidIndex, "Sampler descriptor heap exhausted");
        m_freeSamplerDescriptors.reset(descriptorIndex);
        (lifetime == DescriptorLifetime::kPersistent ? m_persistentSamplerDescriptors : m_transientSamplerDescriptors)
            .set(descriptorIndex);

        if (descriptorIndex == m_samplerDescriptors.size())
            m_samplerDescriptors.push_back(samplerState);
        else
            m_samplerDescriptors[descriptorIndex] = samplerState;
        m_samplerDescriptorMap[key] = descriptorIndex;
        return descriptorIndex;
    }


    void DescriptorManager::UpdateDescriptor(const uint32_t descriptorIndex, const TextureView texture)
    {
        FE_Assert(m_persistentResourceDescriptors.test(descriptorIndex));
        ResourceDescriptorInfo& descriptor = m_resourceDescriptors[descriptorIndex];
        FE_Assert(descriptor.m_resource->GetType() == ResourceType::kTexture);

        RetireResourceDescriptor(descriptorIndex, descriptor.m_resource, false);

        const TextureKey oldKey{ descriptor.m_resource->GetResourceID(),
                                 descriptor.m_textureSubresource,
                                 DescriptorLifetime::kPersistent };
        const DescriptorType descriptorType = descriptor.m_descriptorType;
        m_textureDescriptorMap.erase(oldKey);
        descriptor = ResourceDescriptorInfo{ texture };
        descriptor.m_descriptorType = descriptorType;
        const TextureKey newKey{ texture.m_resource->GetResourceID(), texture.m_subresource, DescriptorLifetime::kPersistent };
        m_textureDescriptorMap[newKey] = descriptorIndex;
        m_resourceDescriptorsToUpdate.set(descriptorIndex);
    }


    void DescriptorManager::UpdateDescriptor(const uint32_t descriptorIndex, const BufferView buffer)
    {
        FE_Assert(m_persistentResourceDescriptors.test(descriptorIndex));
        ResourceDescriptorInfo& descriptor = m_resourceDescriptors[descriptorIndex];
        FE_Assert(descriptor.m_resource->GetType() == ResourceType::kBuffer);

        RetireResourceDescriptor(descriptorIndex, descriptor.m_resource, false);

        const BufferKey oldKey{ descriptor.m_resource->GetResourceID(),
                                descriptor.m_bufferSlice,
                                DescriptorLifetime::kPersistent };
        const DescriptorType descriptorType = descriptor.m_descriptorType;
        m_bufferDescriptorMap.erase(oldKey);
        descriptor = ResourceDescriptorInfo{ buffer };
        descriptor.m_descriptorType = descriptorType;
        const BufferKey newKey{ buffer.m_resource->GetResourceID(), buffer.m_slice, DescriptorLifetime::kPersistent };
        m_bufferDescriptorMap[newKey] = descriptorIndex;
        m_resourceDescriptorsToUpdate.set(descriptorIndex);
    }


    void DescriptorManager::FreeResourceDescriptor(const uint32_t descriptorIndex)
    {
        FE_Assert(m_persistentResourceDescriptors.test(descriptorIndex));
        const ResourceDescriptorInfo& descriptor = m_resourceDescriptors[descriptorIndex];
        RetireResourceDescriptor(descriptorIndex, descriptor.m_resource, true);

        if (descriptor.m_resource->GetType() == ResourceType::kTexture)
        {
            const TextureKey key{ descriptor.m_resource->GetResourceID(),
                                  descriptor.m_textureSubresource,
                                  DescriptorLifetime::kPersistent };
            m_textureDescriptorMap.erase(key);
        }
        else
        {
            const BufferKey key{ descriptor.m_resource->GetResourceID(),
                                 descriptor.m_bufferSlice,
                                 DescriptorLifetime::kPersistent };
            m_bufferDescriptorMap.erase(key);
        }

        m_resourceDescriptors[descriptorIndex] = ResourceDescriptorInfo{};
        m_persistentResourceDescriptors.reset(descriptorIndex);
        m_initializedResourceDescriptors.reset(descriptorIndex);
        m_resourceDescriptorsToUpdate.reset(descriptorIndex);
    }


    void DescriptorManager::FreeSamplerDescriptor(const uint32_t descriptorIndex)
    {
        FE_Assert(m_persistentSamplerDescriptors.test(descriptorIndex));
        RetireSamplerDescriptor(descriptorIndex);
        m_samplerDescriptorMap.erase({ m_samplerDescriptors[descriptorIndex], DescriptorLifetime::kPersistent });
        m_samplerDescriptors[descriptorIndex] = SamplerState::kPointWrap;
        m_persistentSamplerDescriptors.reset(descriptorIndex);
        m_initializedSamplerDescriptors.reset(descriptorIndex);
        m_samplerDescriptorsToUpdate.reset(descriptorIndex);
    }


    void DescriptorManager::CommitResourceDescriptor(const uint32_t descriptorIndex, const DescriptorType type)
    {
        FE_Assert(type == DescriptorType::kSRV || type == DescriptorType::kUAV);

        FE_Assert(!m_freeResourceDescriptors.test(descriptorIndex));
        m_initializedResourceDescriptors.set(descriptorIndex);
        m_resourceDescriptorsToUpdate.set(descriptorIndex);
        m_resourceDescriptors[descriptorIndex].m_descriptorType = type;
    }


    void DescriptorManager::CommitSamplerDescriptor(const uint32_t descriptorIndex)
    {
        FE_Assert(!m_freeSamplerDescriptors.test(descriptorIndex));
        m_initializedSamplerDescriptors.set(descriptorIndex);
        m_samplerDescriptorsToUpdate.set(descriptorIndex);
    }


    ResourceDescriptorInfo DescriptorManager::GetResourceInfo(const uint32_t descriptorIndex)
    {
        FE_Assert(!m_freeResourceDescriptors.test(descriptorIndex));
        return m_resourceDescriptors[descriptorIndex];
    }
} // namespace FE::Graphics::Core
