#pragma once
#include <Core/Math/Frustum.h>
#include <Core/Memory/PoolAllocator.h>
#include <Core/SIMD/Soa.h>
#include <festd/fixed_function.h>

namespace FE
{
    struct Octree;
    struct OctreeNode;
    struct OctreeEntryBlock;


    //! Stable, caller-owned entry. Mutation and traversal must be separate phases.
    struct OctreeEntry final
    {
        [[nodiscard]] Aabb GetBounds() const;

        OctreeNode* m_node = nullptr;
        OctreeEntryBlock* m_block = nullptr;
        uint32_t m_indexInNode = kInvalidIndex;
        uint32_t m_userIndex = kInvalidIndex;
    };


    //! Overflow is block-based so large or coincident entries cannot force unbounded subdivision.
    struct OctreeEntryBlock final
    {
        static constexpr uint32_t kCapacity = 8;

        Simd::Soa::AabbX8 m_bounds{};
        OctreeEntry* m_entries[kCapacity]{};
        OctreeEntryBlock* m_next = nullptr;
        uint32_t m_count = 0;
    };


    struct OctreeNode final
    {
        static constexpr uint32_t kMaxEntryCount = 64;
        static constexpr uint32_t kMaxMergeEntryCount = 32;
        static constexpr uint32_t kMaxDepth = 12;
        static constexpr uint32_t kChildCount = 8;

        [[nodiscard]] const Aabb& GetBounds() const
        {
            return m_bounds;
        }

    private:
        friend Octree;

        Aabb m_bounds = Aabb::kInvalid;
        OctreeNode* m_parent = nullptr;
        Memory::ShortPtr<OctreeNode> m_children = nullptr;
        Memory::ShortPtr<OctreeEntryBlock> m_blocks = nullptr;
        uint32_t m_entryCount = 0;
        uint32_t m_depth = 0;
        uint32_t m_subtreeEntryCount = 0;
    };


    using OctreeTraverseCallback = festd::fixed_function<48, void(const Aabb&, festd::span<OctreeEntry* const>)>;
    using OctreeParallelCallback = festd::fixed_function<48, void(uint32_t taskIndex, festd::span<OctreeEntry* const>)>;


    struct Octree final
    {
        static constexpr uint32_t kMaxTraversalTasks = 64;

        explicit Octree(const Aabb& bounds);
        ~Octree();
        Octree(const Octree&) = delete;
        Octree& operator=(const Octree&) = delete;

        void InsertOrUpdate(OctreeEntry& entry, const Aabb& bounds);
        void Remove(OctreeEntry& entry);

        //! Callbacks receive only intersecting entries. Root overflow is tested even outside the nominal tree bounds.
        void Traverse(const Aabb& bounds, const OctreeTraverseCallback& callback) const;
        void Traverse(const Frustum& bounds, const OctreeTraverseCallback& callback) const;
        //! Synchronous fork/join on the job system. Each task index exclusively owns its callback output.
        void TraverseParallel(const Frustum& bounds, const OctreeParallelCallback& callback) const;

        [[nodiscard]] const Aabb& GetBounds() const
        {
            return m_root.m_bounds;
        }

    private:
        struct NodeChildrenGroup final
        {
            OctreeNode m_nodes[OctreeNode::kChildCount];
        };

        void Insert(OctreeNode& node, OctreeEntry& entry, const Aabb& bounds);
        void InsertLocal(OctreeNode& node, OctreeEntry& entry, const Aabb& bounds);
        void RemoveLocal(OctreeEntry& entry);
        void Split(OctreeNode& node);
        void MergeAncestors(OctreeNode* node);
        void Release(OctreeNode& node);

        template<class TBounds>
        void TraverseNode(const OctreeNode& node, const TBounds& bounds, const OctreeTraverseCallback& callback) const;
        template<class TBounds>
        void TraverseBlocks(const OctreeNode& node, const TBounds& bounds, const OctreeTraverseCallback& callback,
                            const OctreeEntryBlock* first = nullptr, const OctreeEntryBlock* stop = nullptr) const;

        Memory::Pool<NodeChildrenGroup> m_nodePool{ "Math/Octree/Nodes" };
        Memory::Pool<OctreeEntryBlock> m_blockPool{ "Math/Octree/EntryBlocks" };
        OctreeNode m_root;
    };
} // namespace FE
