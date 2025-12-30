#pragma once
#include <FeCore/Containers/Pow2Array.h>
#include <FeCore/Memory/PoolAllocator.h>
#include <FeCore/SIMD/Soa.h>
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
        Aabb m_bounds = Aabb::kInvalid;
        OctreeNode* m_node = nullptr;
        uint32_t m_indexInNode = kInvalidIndex;
        uint32_t m_userIndex = kInvalidIndex;
    };


    struct OctreeNode final
    {
        static constexpr uint32_t kMaxEntryCount = 256;
        static constexpr uint32_t kMaxMergeEntryCount = 64;
        static constexpr uint32_t kChildCount = 8;

        [[nodiscard]] Aabb GetBounds() const;

    private:
        friend Octree;

        void InitRoot(Octree* octree);
        void InitChild(Octree* octree, OctreeNode* parent, uint32_t childIndex);

        [[nodiscard]] bool IsLeaf() const
        {
            return m_children == nullptr;
        }

        template<class TBounds>
        void Traverse(const TBounds& bounds, const OctreeTraverseCallback& callback);

        void Insert(Octree& octree, OctreeEntry* entry);
        void Insert_NoTraverse(OctreeEntry* entry);

        void Update(Octree& octree, OctreeEntry* entry);
        void Remove(Octree& octree, OctreeEntry* entry);

        void Split(Octree& octree);
        void Merge(Octree& octree);

        Simd::Soa::AabbX8 m_childBounds;
        union
        {
            Octree* m_octree = nullptr;
            OctreeNode* m_parent;
        };

        Memory::ShortPtr<OctreeNode> m_children = nullptr;

        bool m_isRoot = false;
        uint8_t m_indexInParent = 0;

        Pow2Array<OctreeEntry*> m_entries;
    };


    struct Octree final
    {
        explicit Octree(const Aabb& bounds);

        void InsertOrUpdate(OctreeEntry& entry);

        void Traverse(const Aabb& bounds, const OctreeTraverseCallback& callback);

    private:
        friend OctreeNode;

        struct NodeDataPool final : public std::pmr::memory_resource
        {
            explicit NodeDataPool(const char* name, uint32_t elementSize, uint32_t elementAlignment);
            ~NodeDataPool() override;

        private:
            static constexpr uint32_t kPoolCount = 8;
            static constexpr uint32_t kMaxAllocationSize = 1 << (kPoolCount - 1);

            uint32_t m_elementSize = 0;
            Memory::PoolAllocator m_allocators[kPoolCount];

            void* do_allocate(size_t byteSize, size_t byteAlignment) override;
            void do_deallocate(void* ptr, size_t byteSize, size_t byteAlignment) override;
            [[nodiscard]] bool do_is_equal(const memory_resource& other) const noexcept override;
        };

        struct NodeChildrenGroup final
        {
            OctreeNode m_nodes[OctreeNode::kChildCount];
        };

        static_assert(std::is_standard_layout_v<NodeChildrenGroup>);

        template<class TBounds>
        void TraverseImpl(const TBounds& bounds, const OctreeTraverseCallback& callback);

        NodeDataPool m_nodeEntriesPool{ "Graphics/Scene/Octree/NodeEntriesPool", sizeof(OctreeEntry*), alignof(OctreeEntry*) };

        Memory::Pool<NodeChildrenGroup> m_nodePool{ "Graphics/Scene/Octree/NodePool" };

        Aabb m_bounds;
        OctreeNode m_root;
    };
} // namespace FE::Graphics
