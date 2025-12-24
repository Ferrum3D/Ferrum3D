#include <FeCore/SIMD/Intersection.h>
#include <Graphics/Scene/Octree.h>

namespace FE::Graphics
{
    namespace
    {
        template<class TBounds>
        uint32_t CullBounds(const TBounds& test, const festd::span<const Simd::Soa::AabbX8> bounds,
                            const festd::span<OctreeEntry*> entries, OctreeEntry** output)
        {
            uint32_t outputIndex = 0;
            for (uint32_t boundGroupIndex = 0; boundGroupIndex < bounds.size(); ++boundGroupIndex)
            {
                const Simd::Soa::MaskX8 overlapMaskVector = Simd::Intersection::Overlaps(test, bounds[boundGroupIndex]);

                uint32_t overlapMask = Simd::Soa::MoveMask(overlapMaskVector);
                uint32_t entryLocalIndex;
                while (Bit::ScanForward(entryLocalIndex, overlapMask))
                {
                    output[outputIndex++] = entries[entryLocalIndex + boundGroupIndex * Simd::AVX::kLaneCount];
                    overlapMask &= ~(1 << entryLocalIndex);
                }
            }

            return outputIndex;
        }
    } // namespace


    template<bool TPrecise, class TBounds>
    void OctreeNode::Traverse(const TBounds& bounds, const OctreeTraverseCallback& callback, OctreeEntry** tempBuffer)
    {
        FE_AssertDebug(!IsLeaf());

        const Simd::Soa::MaskX8 overlapMaskVector = Simd::Intersection::Overlaps(bounds, m_childBounds);

        uint32_t childIndex;
        uint32_t overlapMask = Simd::Soa::MoveMask(overlapMaskVector);
        while (Bit::ScanForward(childIndex, overlapMask))
        {
            OctreeNode& child = m_children[childIndex];

            if (!child.m_entries.empty())
            {
                if constexpr (TPrecise)
                {
                    const uint32_t entryCount = CullBounds(bounds, child.m_childBounds, child.m_entries, tempBuffer);
                    callback(child.m_bounds, festd::span(tempBuffer, entryCount));
                }
                else
                {
                    callback(child.m_bounds, child.m_entries);
                }
            }

            if (!child.IsLeaf())
                child.Traverse(bounds, callback);

            overlapMask &= ~(1 << childIndex);
        }
    }


    template<bool TPrecise, class TBounds>
    void Octree::TraverseImpl(const TBounds& bounds, const OctreeTraverseCallback& callback, OctreeEntry** tempBuffer)
    {
        if (Math::Overlaps(bounds, m_root.m_bounds))
        {
            if (!m_root.m_entries.empty())
            {
                if constexpr (TPrecise)
                {
                    const uint32_t entryCount = CullBounds(bounds, m_root.m_childBounds, m_root.m_entries, tempBuffer);
                    callback(m_root.m_bounds, festd::span(tempBuffer, entryCount));
                }
                else
                {
                    callback(m_root.m_bounds, m_root.m_entries);
                }
            }

            if (!m_root.IsLeaf())
            {
                const auto boundsBroadcast = Simd::Soa::Broadcast(bounds);
                m_root.Traverse<TPrecise, TBounds>(boundsBroadcast, callback);
            }
        }
    }


    void OctreeNode::Init(Octree* octree, const Aabb& bounds)
    {
        m_bounds = bounds;
        m_entries.set_allocator(&octree->m_nodeEntriesPool);
        m_entryBounds.set_allocator(&octree->m_nodeEntryBoundsPool);
    }


    void OctreeNode::Insert(Octree& octree, OctreeEntry* entry, const Aabb& bounds) {}


    void OctreeNode::Update(Octree& octree, OctreeEntry* entry, const Aabb& bounds)
    {
        if (IsLeaf() && Math::Overlaps(m_bounds, bounds))
            return;
    }


    void OctreeNode::Split(Octree& octree) {}


    Octree::Octree(const Aabb& bounds)
    {
        m_root.Init(this, bounds);
    }


    void Octree::InsertOrUpdate(OctreeEntry& entry, const Aabb& bounds)
    {
        if (entry.m_node)
            entry.m_node->Update(*this, &entry, bounds);
    }


    void Octree::Traverse(const Aabb& bounds, const OctreeTraverseCallback& callback, const bool preciseCulling)
    {
        OctreeEntry* tempBuffer[OctreeNode::kMaxEntryCount];

        if (preciseCulling)
            TraverseImpl<true>(bounds, callback, tempBuffer);
        else
            TraverseImpl<false>(bounds, callback, tempBuffer);
    }


    Octree::NodeDataPool::NodeDataPool(const char* name, const uint32_t elementSize)
    {
        for (uint32_t allocatorIndex = 0; allocatorIndex < festd::size(m_allocators); ++allocatorIndex)
        {
            const uint32_t elementCount = 1 << allocatorIndex;
            m_allocators[allocatorIndex].Initialize(name, static_cast<size_t>(elementSize) * elementCount);
        }
    }


    Octree::NodeDataPool::~NodeDataPool()
    {
        for (Memory::PoolAllocator& allocator : m_allocators)
            allocator.Deinitialize(true);
    }
} // namespace FE::Graphics
