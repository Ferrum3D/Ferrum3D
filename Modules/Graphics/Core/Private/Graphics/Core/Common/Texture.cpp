#include <Graphics/Core/Common/Texture.h>

namespace FE::Graphics::Common
{
    Core::ResourceMemory Texture::GetMemoryStatus() const
    {
        if (m_instance == nullptr)
            return Core::ResourceMemory::kNotCommitted;

        return m_instance->m_memoryStatus;
    }


    void Texture::SetState(const Core::TextureSubresource subresource, const SubresourceState state)
    {
        std::unique_lock lk{ m_lock };

        FE_Assert(m_instance);

        auto& subresourceStates = m_instance->m_subresourceStates;
        if (subresourceStates.size() == 1)
        {
            if (subresource == m_wholeImageSubresource)
            {
                subresourceStates[0] = state;
                return;
            }

            subresourceStates.resize(m_desc.m_mipSliceCount * m_desc.m_arraySize, subresourceStates[0]);
        }

        const Core::TextureSubresourceIterator subresourceIterator{ subresource };
        for (const auto [mipIndex, arrayIndex] : subresourceIterator)
            subresourceStates[mipIndex * m_desc.m_arraySize + arrayIndex] = state;
    }


    SubresourceState Texture::GetState(const Core::TextureSubresource subresource) const
    {
        std::unique_lock lk{ m_lock };

        FE_Assert(m_instance);

        FE_AssertDebug(subresource.m_mipSliceCount == 1);
        FE_AssertDebug(subresource.m_arraySize == 1);

        auto& subresourceStates = m_instance->m_subresourceStates;
        if (subresourceStates.size() == 1)
            return subresourceStates[0];

        return subresourceStates[subresource.m_mostDetailedMipSlice * m_desc.m_arraySize + subresource.m_firstArraySlice];
    }


    void Texture::AddQueueReleaseBarrier(const Core::TextureBarrierDesc& barrier)
    {
        std::unique_lock lk{ m_lock };

        FE_Assert(barrier.m_queueBefore != barrier.m_queueAfter);
        auto& barriers = m_queueReleaseBarriers[festd::to_underlying(barrier.m_queueAfter)];
        barriers.push_back(barrier);
    }


    festd::optional<Core::TextureBarrierDesc> Texture::RetrieveQueueReleaseBarrier(const Core::DeviceQueueType receiverQueue,
                                                                                   const Core::TextureSubresource subresource)
    {
        std::unique_lock lk{ m_lock };

        auto& barriers = m_queueReleaseBarriers[festd::to_underlying(receiverQueue)];
        const auto it = festd::find_if(barriers, [subresource](const Core::TextureBarrierDesc& barrier) {
            return barrier.m_subresource.Contains(subresource);
        });

        if (it == barriers.end())
            return festd::nullopt;

        barriers.erase(it);
        return *it;
    }


    void Texture::SetQueueOwnership(const Core::TextureSubresource subresource, const Core::DeviceQueueType queue)
    {
        std::unique_lock lk{ m_lock };

        FE_Assert(m_instance);

        auto& subresourceStates = m_instance->m_subresourceStates;
        if (subresourceStates.size() == 1)
        {
            if (subresource == m_wholeImageSubresource)
            {
                subresourceStates[0].m_queueType = queue;
                return;
            }

            subresourceStates.resize(m_desc.m_mipSliceCount * m_desc.m_arraySize, subresourceStates[0]);
        }

        const Core::TextureSubresourceIterator subresourceIterator{ subresource };
        for (const auto [mipIndex, arrayIndex] : subresourceIterator)
            subresourceStates[mipIndex * m_desc.m_arraySize + arrayIndex].m_queueType = queue;
    }
} // namespace FE::Graphics::Common
