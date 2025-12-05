#include <Graphics/Core/DescriptorManager.h>

namespace FE::Graphics::Core
{
    ResourceDescriptorInfo::ResourceDescriptorInfo(Texture* texture, const TextureSubresource subresource)
    {
        m_resource = texture;
        m_textureSubresource = subresource;
    }


    ResourceDescriptorInfo::ResourceDescriptorInfo(Buffer* buffer, const BufferSubresource subresource)
    {
        m_resource = buffer;
        m_bufferSubresource = subresource;
    }


    DescriptorManager::DescriptorManager()
    {
        m_committedResourceDescriptors.resize(kResourceDescriptorCount, false);
        m_committedSamplerDescriptors.resize(kSamplerDescriptorCount, false);
    }


    void DescriptorManager::Clear()
    {
        m_committedResourceDescriptors.reset();
        m_committedSamplerDescriptors.reset();

        m_resourceDescriptors.clear();
        m_samplerDescriptors.clear();

        m_textureDescriptorMap.clear();
        m_bufferDescriptorMap.clear();
        m_samplerDescriptorMap.clear();
    }


    uint32_t DescriptorManager::ReserveDescriptor(Texture* texture, const TextureSubresource subresource)
    {
        TextureKey key;
        key.m_resourceID = texture->GetResourceID();
        key.m_subresource = subresource;

        const auto it = m_textureDescriptorMap.find(key);
        if (it != m_textureDescriptorMap.end())
            return it->second;

        const uint32_t descriptorIndex = m_resourceDescriptors.size();
        m_resourceDescriptors.push_back({ texture, subresource });
        m_textureDescriptorMap[key] = descriptorIndex;
        return descriptorIndex;
    }


    uint32_t DescriptorManager::ReserveDescriptor(Buffer* buffer, const BufferSubresource subresource)
    {
        BufferKey key;
        key.m_resourceID = buffer->GetResourceID();
        key.m_subresource = subresource;

        const auto it = m_bufferDescriptorMap.find(key);
        if (it != m_bufferDescriptorMap.end())
            return it->second;

        const uint32_t descriptorIndex = m_resourceDescriptors.size();
        m_resourceDescriptors.push_back({ buffer, subresource });
        m_bufferDescriptorMap[key] = descriptorIndex;
        return descriptorIndex;
    }


    uint32_t DescriptorManager::ReserveDescriptor(const SamplerState samplerState)
    {
        const auto it = m_samplerDescriptorMap.find(samplerState);
        if (it != m_samplerDescriptorMap.end())
            return it->second;

        const uint32_t descriptorIndex = m_samplerDescriptors.size();
        m_samplerDescriptors.push_back(samplerState);
        m_samplerDescriptorMap[samplerState] = descriptorIndex;
        return descriptorIndex;
    }


    void DescriptorManager::CommitResourceDescriptor(const uint32_t descriptorIndex, const DescriptorType type)
    {
        FE_Assert(type == DescriptorType::kSRV || type == DescriptorType::kUAV);

        m_committedResourceDescriptors.set(descriptorIndex);
        m_resourceDescriptors[descriptorIndex].m_descriptorType = type;
    }


    void DescriptorManager::CommitSamplerDescriptor(const uint32_t descriptorIndex)
    {
        m_committedSamplerDescriptors.set(descriptorIndex);
    }


    ResourceDescriptorInfo DescriptorManager::GetResourceInfo(const uint32_t descriptorIndex)
    {
        return m_resourceDescriptors[descriptorIndex];
    }
} // namespace FE::Graphics::Core
