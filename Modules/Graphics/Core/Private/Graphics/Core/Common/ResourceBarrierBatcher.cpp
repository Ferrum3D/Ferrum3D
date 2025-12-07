#include <Graphics/Core/Common/ResourceBarrierBatcher.h>

namespace FE::Graphics::Common
{
    namespace
    {
        bool AreBarriersCompatible(const Core::TextureBarrierDesc& a, const Core::TextureBarrierDesc& b)
        {
            if (a.m_texture != b.m_texture)
                return false;

            if (a.m_layoutAfter != b.m_layoutAfter)
                return false;

            if (a.m_layoutBefore != b.m_layoutBefore)
                return false;

            if (a.m_accessBefore != b.m_accessBefore)
                return false;

            if (a.m_syncBefore != b.m_syncBefore)
                return false;

            return true;
        }


        bool SafeToCoalesce(const Core::TextureSubresource a, const Core::TextureSubresource b)
        {
            if (a.m_aspect != b.m_aspect)
                return false;

            const uint32_t aMipStart = a.m_mostDetailedMipSlice;
            const uint32_t aMipEnd = a.m_mostDetailedMipSlice + a.m_mipSliceCount;
            const uint32_t bMipStart = b.m_mostDetailedMipSlice;
            const uint32_t bMipEnd = b.m_mostDetailedMipSlice + b.m_mipSliceCount;

            const bool mipCompatible = (aMipStart <= bMipEnd && bMipStart <= aMipEnd) || //
                aMipEnd == bMipStart || bMipEnd == aMipStart;

            if (!mipCompatible)
                return false;

            const uint32_t aArrayStart = a.m_firstArraySlice;
            const uint32_t aArrayEnd = a.m_firstArraySlice + a.m_arraySize;
            const uint32_t bArrayStart = b.m_firstArraySlice;
            const uint32_t bArrayEnd = b.m_firstArraySlice + b.m_arraySize;

            const bool arrayCompatible = (aArrayStart <= bArrayEnd && bArrayStart <= aArrayEnd) || //
                aArrayEnd == bArrayStart || bArrayEnd == aArrayStart;

            return arrayCompatible;
        }
    } // namespace


    ResourceBarrierBatcher::ResourceBarrierBatcher(std::pmr::memory_resource* allocator)
        : m_textureBarriers(allocator)
        , m_bufferBarriers(allocator)
    {
    }


    bool ResourceBarrierBatcher::AddBarrier(const Core::TextureBarrierDesc& barrier)
    {
        FE_Assert(barrier.m_queueBefore == barrier.m_queueAfter, "Ownership transfer barriers cannot be batched");

        // Firstly, group mergeable barriers together.
        const auto compareBarriers = [](const Core::TextureBarrierDesc& existing, const Core::TextureBarrierDesc& added) {
            // We can't merge barriers with different resources or layouts.
            if (existing.m_texture != added.m_texture)
                return existing.m_texture < added.m_texture;

            if (existing.m_layoutAfter != added.m_layoutAfter)
                return existing.m_layoutAfter < added.m_layoutAfter;

            // We also can't merge them if their initial layouts or access flags were different.
            if (existing.m_layoutBefore != added.m_layoutBefore)
                return existing.m_layoutBefore < added.m_layoutBefore;

            if (existing.m_accessBefore != added.m_accessBefore)
                return existing.m_accessBefore < added.m_accessBefore;

            if (existing.m_syncBefore != added.m_syncBefore)
                return existing.m_syncBefore < added.m_syncBefore;

            FE_AssertDebug(Core::IsReadAccess(existing.m_accessAfter) && Core::IsReadAccess(added.m_accessAfter)
                           || Core::IsWriteAccess(existing.m_accessAfter) && Core::IsWriteAccess(added.m_accessAfter));

            if (Core::IsReadAccess(existing.m_accessAfter) != Core::IsReadAccess(added.m_accessAfter))
                return Core::IsReadAccess(existing.m_accessAfter);

            // Reads are always mergeable as long as the layouts are the same.
            // For writes, however, we need to make sure that the access flags are the same.
            if (!Core::IsReadAccess(existing.m_accessAfter) && existing.m_accessAfter != added.m_accessAfter)
                return existing.m_accessAfter < added.m_accessAfter;

            if (!Core::IsReadAccess(existing.m_accessAfter) && existing.m_syncAfter != added.m_syncAfter)
                return existing.m_syncAfter < added.m_syncAfter;

            if (existing.m_subresource.m_mostDetailedMipSlice != added.m_subresource.m_mostDetailedMipSlice)
                return existing.m_subresource.m_mostDetailedMipSlice < added.m_subresource.m_mostDetailedMipSlice;

            return existing.m_subresource.m_firstArraySlice < added.m_subresource.m_firstArraySlice;
        };

        const auto it = festd::lower_bound(m_textureBarriers, barrier, compareBarriers);
        if (it != m_textureBarriers.end() && it->m_texture == barrier.m_texture && AreBarriersCompatible(*it, barrier))
        {
            Core::TextureBarrierDesc& existingBarrier = *it;
            if (SafeToCoalesce(existingBarrier.m_subresource, barrier.m_subresource))
            {
                if (Core::IsReadAccess(existingBarrier.m_accessAfter) && Core::IsReadAccess(barrier.m_accessAfter))
                {
                    existingBarrier.m_subresource = existingBarrier.m_subresource.Coalesce(barrier.m_subresource);
                    existingBarrier.m_accessAfter |= barrier.m_accessAfter;
                    existingBarrier.m_syncAfter |= barrier.m_syncAfter;
                    return true;
                }

                if (existingBarrier.m_accessAfter == barrier.m_accessAfter && existingBarrier.m_syncAfter == barrier.m_syncAfter)
                {
                    existingBarrier.m_subresource = existingBarrier.m_subresource.Coalesce(barrier.m_subresource);
                    return true;
                }
            }
        }

        m_textureBarriers.insert(it, barrier);
        return false;
    }


    bool ResourceBarrierBatcher::AddBarrier(const Core::BufferBarrierDesc& barrier)
    {
        FE_Assert(barrier.m_queueBefore == barrier.m_queueAfter, "Ownership transfer barriers cannot be batched");

        for (Core::BufferBarrierDesc& existingBarrier : m_bufferBarriers)
        {
            if (existingBarrier.m_buffer == barrier.m_buffer)
            {
                if (Core::IsReadAccess(existingBarrier.m_accessAfter) && Core::IsReadAccess(barrier.m_accessAfter))
                {
                    existingBarrier.m_accessAfter |= barrier.m_accessAfter;
                    existingBarrier.m_syncAfter |= barrier.m_syncAfter;
                    return true;
                }
            }
        }

        m_bufferBarriers.push_back(barrier);
        return false;
    }
} // namespace FE::Graphics::Common
