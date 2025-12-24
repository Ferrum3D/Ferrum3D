#pragma once
#include <FeCore/Containers/Pow2Array.h>
#include <FeCore/Memory/PoolAllocator.h>
#include <FeCore/SIMD/Soa.h>
#include <Graphics/Scene/Octree.h>
#include <festd/fixed_function.h>
#include <festd/vector.h>

namespace FE::Graphics
{
    struct Octree;
    struct OctreeNode;
    struct OctreeEntry;


    using OctreeTraverseCallback = festd::fixed_function<48, void(const Aabb& nodeBounds, festd::span<OctreeEntry*> nodeEntries)>;


    struct OctreeEntry final
    {
        OctreeNode* m_node = nullptr;
        uint32_t m_userIndex = kInvalidIndex;
    };


    struct OctreeNode final
    {
        static constexpr uint32_t kMaxEntryCount = 256;
        static constexpr uint32_t kEntryArraySizeClasses = 9;
        static constexpr uint32_t kChildCount = 8;

    private:
        friend Octree;

        void Init(Octree* octree, const Aabb& bounds);

        static_assert(kMaxEntryCount == 1 << (kEntryArraySizeClasses - 1));

        [[nodiscard]] bool IsLeaf() const
        {
            return m_children != nullptr;
        }

        template<bool TPrecise, class TBounds>
        void Traverse(const TBounds& bounds, const OctreeTraverseCallback& callback, OctreeEntry** tempBuffer);

        void Insert(Octree& octree, OctreeEntry* entry, const Aabb& bounds);
        void Update(Octree& octree, OctreeEntry* entry, const Aabb& bounds);
        void Split(Octree& octree);

        Aabb m_bounds = Aabb::kInvalid;
        Simd::Soa::AabbX8 m_childBounds;
        OctreeNode* m_children = nullptr;

        Pow2Array<OctreeEntry*> m_entries;
        Pow2Array<Simd::Soa::AabbX8> m_entryBounds;
    };


    struct Octree final
    {
        explicit Octree(const Aabb& bounds);

        void InsertOrUpdate(OctreeEntry& entry, const Aabb& bounds);

        void Traverse(const Aabb& bounds, const OctreeTraverseCallback& callback, bool preciseCulling = false);

    private:
        friend OctreeNode;

        struct NodeDataPool final : public std::pmr::memory_resource
        {
            explicit NodeDataPool(const char* name, uint32_t elementSize);
            ~NodeDataPool() override;

        private:
            Memory::PoolAllocator m_allocators[OctreeNode::kEntryArraySizeClasses];

            void* do_allocate(size_t bytes, size_t align) override;
            void do_deallocate(void* ptr, size_t bytes, size_t align) override;
            [[nodiscard]] bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override;
        };

        struct NodeChildrenGroup final
        {
            OctreeNode m_nodes[OctreeNode::kChildCount];
        };

        template<bool TPrecise, class TBounds>
        void TraverseImpl(const TBounds& bounds, const OctreeTraverseCallback& callback, OctreeEntry** tempBuffer);

        NodeDataPool m_nodeEntriesPool{ "Graphics/Scene/Octree/NodeEntriesPool", sizeof(OctreeEntry*) };
        NodeDataPool m_nodeEntryBoundsPool{ "Graphics/Scene/Octree/NodeEntryBoundsPool", sizeof(Simd::Soa::AabbX8) };

        Memory::Pool<NodeChildrenGroup> m_nodePool{ "Graphics/Scene/Octree/NodePool" };
        OctreeNode m_root;
    };
} // namespace FE::Graphics
