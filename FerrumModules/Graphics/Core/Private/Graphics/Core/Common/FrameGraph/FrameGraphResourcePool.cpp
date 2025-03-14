#include <Graphics/Core/Common/FrameGraph/FrameGraphResourcePool.h>

namespace FE::Graphics::Common
{
    FrameGraphResourcePool::FrameGraphResourcePool(Core::ResourcePool* pool)
        : m_resourcePool(pool)
    {
    }


    void FrameGraphResourcePool::Reset()
    {
        FE_PROFILER_ZONE();

        m_imagesMap.clear();
        m_buffersMap.clear();

        for (const ImageInfo& imageInfo : m_createdImages)
            m_imagesMap[imageInfo.m_descHash] = imageInfo.m_image;
        for (const BufferInfo& bufferInfo : m_createdBuffers)
            m_buffersMap[bufferInfo.m_descHash] = bufferInfo.m_buffer;

        m_createdImages.clear();
        m_createdBuffers.clear();
    }


    Core::Image* FrameGraphResourcePool::CreateImage(const Env::Name name, const Core::ImageDesc& desc)
    {
        FE_PROFILER_ZONE();

        const uint64_t descHash = desc.GetHash();

        if (const auto iter = m_imagesMap.find(descHash); iter != m_imagesMap.end())
        {
            m_imagesMap.erase(iter);
            return iter->second.Get();
        }

        const auto result = m_resourcePool->CreateImage(name, desc);
        ImageInfo& createdImageInfo = m_createdImages.push_back();
        createdImageInfo.m_image = result;
        createdImageInfo.m_descHash = descHash;

        return result;
    }


    Core::Buffer* FrameGraphResourcePool::CreateBuffer(const Env::Name name, const Core::BufferDesc& desc)
    {
        FE_PROFILER_ZONE();

        const uint64_t descHash = desc.GetHash();

        if (const auto iter = m_buffersMap.find(descHash); iter != m_buffersMap.end())
        {
            m_buffersMap.erase(iter);
            return iter->second.Get();
        }

        const auto result = m_resourcePool->CreateBuffer(name, desc);
        BufferInfo& createdBufferInfo = m_createdBuffers.push_back();
        createdBufferInfo.m_buffer = result;
        createdBufferInfo.m_descHash = descHash;

        return result;
    }
} // namespace FE::Graphics::Common
