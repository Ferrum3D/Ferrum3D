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

        // TODO: Clean up old resources
    }


    Core::RenderTarget* FrameGraphResourcePool::CreateRenderTarget(const Env::Name name, const Core::ImageDesc& desc)
    {
        FE_PROFILER_ZONE();

        const uint64_t descHash = desc.GetHash();

        if (const auto iter = m_imagesMap.find(descHash); iter != m_imagesMap.end())
            return iter->second.Get();

        Core::RenderTarget* result = m_resourcePool->CreateRenderTarget(name, desc);
        m_imagesMap[descHash] = result;

        return result;
    }


    Core::Buffer* FrameGraphResourcePool::CreateBuffer(const Env::Name name, const Core::BufferDesc& desc)
    {
        FE_PROFILER_ZONE();

        const uint64_t descHash = desc.GetHash();

        if (const auto iter = m_buffersMap.find(descHash); iter != m_buffersMap.end())
            return iter->second.Get();

        Core::Buffer* result = m_resourcePool->CreateBuffer(name, desc);
        m_buffersMap[descHash] = result;

        return result;
    }
} // namespace FE::Graphics::Common
