#include <FeCore/SIMD/Intersection.h>
#include <Graphics/Scene/Octree.h>

namespace FE::Graphics
{
    template<class TBounds>
    void OctreeNode::Traverse(const TBounds& bounds, const OctreeTraverseCallback& callback)
    {
        FE_AssertDebug(!IsLeaf());

        OctreeNode* children = m_children.Get();

        const Simd::Soa::MaskX8 overlapMaskVector = Simd::Intersection::Overlaps(bounds, m_childBounds);

        uint32_t childIndex;
        uint32_t overlapMask = Simd::Soa::MoveMask(overlapMaskVector);
        while (Bit::ScanForward(childIndex, overlapMask))
        {
            OctreeNode& child = children[childIndex];

            if (!child.m_entries.empty())
            {
                const Aabb childBounds = Simd::Soa::Extract(m_childBounds, childIndex);
                callback(childBounds, child.m_entries);
            }

            if (!child.IsLeaf())
                child.Traverse(bounds, callback);

            overlapMask &= ~(1 << childIndex);
        }
    }


    template<class TBounds>
    void Octree::TraverseImpl(const TBounds& bounds, const OctreeTraverseCallback& callback)
    {
        if (Math::Overlaps(bounds, m_bounds))
        {
            if (!m_root.m_entries.empty())
                callback(m_bounds, m_root.m_entries);

            if (!m_root.IsLeaf())
            {
                const auto boundsBroadcast = Simd::Soa::Broadcast(bounds);
                m_root.Traverse<TBounds>(boundsBroadcast, callback);
            }
        }
    }


    Aabb OctreeNode::GetBounds() const
    {
        if (m_isRoot)
            return m_octree->m_bounds;

        return Simd::Soa::Extract(m_parent->m_childBounds, m_indexInParent);
    }


    void OctreeNode::InitRoot(Octree* octree)
    {
        m_isRoot = true;
        m_octree = octree;
        m_entries.set_allocator(&octree->m_nodeEntriesPool);
    }


    void OctreeNode::InitChild(Octree* octree, OctreeNode* parent, const uint32_t childIndex)
    {
        m_isRoot = false;
        m_parent = parent;
        m_indexInParent = static_cast<uint8_t>(childIndex);
        m_entries.set_allocator(&octree->m_nodeEntriesPool);
    }


    void OctreeNode::Insert(Octree& octree, OctreeEntry* entry)
    {
        if (!IsLeaf())
        {
            const Simd::Soa::AabbX8 entryBounds = Simd::Soa::Broadcast(entry->m_bounds);
            const Simd::Soa::MaskX8 containingMaskVector = Simd::Intersection::Contains(m_childBounds, entryBounds);

            const uint32_t containingMask = Simd::Soa::MoveMask(containingMaskVector);
            if (uint32_t childIndex; Bit::ScanForward(childIndex, containingMask))
            {
                OctreeNode& child = m_children[childIndex];
                child.Insert(octree, entry);
                return;
            }
        }

        if (IsLeaf() && m_entries.size() >= kMaxEntryCount)
        {
            Split(octree);
            Insert(octree, entry);
            return;
        }

        Insert_NoTraverse(entry);
    }


    void OctreeNode::Insert_NoTraverse(OctreeEntry* entry)
    {
        entry->m_indexInNode = m_entries.size();
        entry->m_node = this;
        m_entries.push_back(entry);
    }


    void OctreeNode::Update(Octree& octree, OctreeEntry* entry)
    {
        const Aabb entryBounds = entry->m_bounds;
        if (IsLeaf() && Math::Contains(GetBounds(), entryBounds))
            return;

        if (m_isRoot)
            return;

        Remove(octree, entry);

        OctreeNode* node = m_parent;
        while (node)
        {
            if (Math::Contains(node->GetBounds(), entryBounds) || node->m_isRoot)
            {
                node->Insert_NoTraverse(entry);
                return;
            }

            node = node->m_parent;
        }
    }


    void OctreeNode::Remove(Octree& octree, OctreeEntry* entry)
    {
        FE_AssertDebug(entry->m_node == this);
        FE_AssertDebug(m_entries[entry->m_indexInNode] == entry);

        entry->m_node = nullptr;
        entry->m_indexInNode = kInvalidIndex;

        m_entries.erase_unsorted(m_entries.begin() + entry->m_indexInNode);

        if (!m_isRoot)
            m_parent->Merge(octree);
    }


    void OctreeNode::Split(Octree& octree)
    {
        FE_AssertDebug(m_children == nullptr);

        OctreeNode* children = octree.m_nodePool.New()->m_nodes;
        m_children = children;

        const Aabb bounds = GetBounds();
        const Vector3 childExtent = (bounds.max - bounds.min) * 0.5f;

        union ChildOffsets
        {
            struct
            {
                alignas(Simd::AVX::kByteSize) float x[Simd::AVX::kLaneCount];
                alignas(Simd::AVX::kByteSize) float y[Simd::AVX::kLaneCount];
                alignas(Simd::AVX::kByteSize) float z[Simd::AVX::kLaneCount];
            };

            Simd::Soa::Vector3X8 vector;
        } childOffsets;

        static_assert(kChildCount == Simd::AVX::kLaneCount);
        static_assert(sizeof(ChildOffsets) == sizeof(Simd::Soa::Vector3X8));
        static_assert(offsetof(ChildOffsets, x) == offsetof(Simd::Soa::Vector3X8, x));
        static_assert(offsetof(ChildOffsets, y) == offsetof(Simd::Soa::Vector3X8, y));
        static_assert(offsetof(ChildOffsets, z) == offsetof(Simd::Soa::Vector3X8, z));

        Simd::Soa::SetZero(childOffsets.vector);

        for (uint32_t childIndex = 0; childIndex < kChildCount; ++childIndex)
        {
            OctreeNode& child = children[childIndex];
            child.InitChild(&octree, this, childIndex);

            if (childIndex & 0x1)
                childOffsets.x[childIndex] = childExtent.x;

            if (childIndex & 0x2)
                childOffsets.y[childIndex] = childExtent.y;

            if (childIndex & 0x4)
                childOffsets.z[childIndex] = childExtent.z;
        }

        const Simd::Soa::Vector3X8 minBroadcast = Simd::Soa::Broadcast(bounds.min);
        const Simd::Soa::Vector3X8 childExtentBroadcast = Simd::Soa::Broadcast(childExtent);

        m_childBounds.min = Simd::Soa::Add(minBroadcast, childOffsets.vector);
        m_childBounds.max = Simd::Soa::Add(Simd::Soa::Add(minBroadcast, childExtentBroadcast), childOffsets.vector);

        Pow2Array<OctreeEntry*> entries(&octree.m_nodeEntriesPool);
        festd::swap(entries, m_entries);

        for (OctreeEntry* entry : entries)
        {
            entry->m_node = nullptr;
            entry->m_indexInNode = kInvalidIndex;
            Insert(octree, entry);
        }
    }


    void OctreeNode::Merge(Octree& octree)
    {
        FE_AssertDebug(!IsLeaf());

        OctreeNode* children = m_children.Get();

        uint32_t entriesAfterMerge = m_entries.size();
        for (uint32_t childIndex = 0; childIndex < kChildCount; ++childIndex)
        {
            OctreeNode& child = children[childIndex];

            if (child.IsLeaf())
                continue;

            child.Merge(octree);
            if (!child.IsLeaf())
                return;

            entriesAfterMerge += child.m_entries.size();
        }

        if (entriesAfterMerge > kMaxMergeEntryCount)
            return;

        for (uint32_t childIndex = 0; childIndex < kChildCount; ++childIndex)
        {
            OctreeNode& child = children[childIndex];

            for (OctreeEntry* entry : child.m_entries)
            {
                entry->m_node = this;
                entry->m_indexInNode = m_entries.size();
                m_entries.push_back(entry);
            }
        }

        octree.m_nodePool.Delete(reinterpret_cast<Octree::NodeChildrenGroup*>(m_children.Get()));
        m_children = nullptr;
    }


    Octree::Octree(const Aabb& bounds)
    {
        m_bounds = bounds;
        m_root.InitRoot(this);
    }


    void Octree::InsertOrUpdate(OctreeEntry& entry)
    {
        if (entry.m_node)
            entry.m_node->Update(*this, &entry);
    }


    void Octree::Traverse(const Aabb& bounds, const OctreeTraverseCallback& callback)
    {
        TraverseImpl(bounds, callback);
    }


    Octree::NodeDataPool::NodeDataPool(const char* name, const uint32_t elementSize, const uint32_t elementAlignment)
        : m_elementSize(elementSize)
    {
        for (uint32_t allocatorIndex = 0; allocatorIndex < festd::size(m_allocators); ++allocatorIndex)
        {
            const uint32_t elementCount = 1 << allocatorIndex;
            m_allocators[allocatorIndex].Initialize(name, elementSize * elementCount, elementAlignment);
        }
    }


    Octree::NodeDataPool::~NodeDataPool()
    {
        for (Memory::PoolAllocator& allocator : m_allocators)
            allocator.Deinitialize(true);
    }


    void* Octree::NodeDataPool::do_allocate(const size_t byteSize, const size_t byteAlignment)
    {
        FE_AssertDebug(byteSize > 0);
        FE_AssertDebug(byteSize % m_elementSize == 0);

        const uint32_t elementCount = static_cast<uint32_t>(byteSize) / m_elementSize;
        const uint32_t allocatorIndex = Math::FloorLog2(elementCount);
        if (allocatorIndex > festd::size(m_allocators))
            return Memory::DefaultAllocate(byteSize, byteAlignment);

        FE_AssertDebug(m_allocators[allocatorIndex].GetElementByteSize() == byteSize);
        FE_AssertDebug(m_allocators[allocatorIndex].GetElementAlignment() == byteAlignment);

        return m_allocators[allocatorIndex].allocate(byteSize, byteAlignment);
    }


    void Octree::NodeDataPool::do_deallocate(void* ptr, const size_t byteSize, const size_t byteAlignment)
    {
        FE_AssertDebug(byteSize > 0);
        FE_AssertDebug(byteSize % m_elementSize == 0);

        const uint32_t elementCount = static_cast<uint32_t>(byteSize) / m_elementSize;
        const uint32_t allocatorIndex = Math::FloorLog2(elementCount);
        if (allocatorIndex > festd::size(m_allocators))
        {
            Memory::DefaultFree(ptr);
            return;
        }

        FE_AssertDebug(m_allocators[allocatorIndex].GetElementByteSize() == byteSize);
        FE_AssertDebug(m_allocators[allocatorIndex].GetElementAlignment() == byteAlignment);

        m_allocators[allocatorIndex].deallocate(ptr, byteSize, byteAlignment);
    }


    bool Octree::NodeDataPool::do_is_equal(const memory_resource& other) const noexcept
    {
        return this == &other;
    }
} // namespace FE::Graphics
