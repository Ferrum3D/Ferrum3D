#include <Core/Jobs/Jobs.h>
#include <Core/Math/Octree.h>
#include <Core/SIMD/Intersection.h>
#include <bit>
#include <festd/vector.h>

namespace FE
{
    namespace
    {
        bool Overlaps(const Frustum& bounds, const Aabb& box)
        {
            return bounds.Overlaps(box);
        }


        bool Overlaps(const Aabb& bounds, const Aabb& box)
        {
            return Math::Overlaps(bounds, box);
        }


        uint32_t OverlapMask(const Frustum& bounds, const Simd::Soa::AabbX8& boxes)
        {
            return Simd::Soa::MoveMask(Simd::Intersection::Overlaps(bounds, boxes));
        }


        uint32_t OverlapMask(const Aabb& bounds, const Simd::Soa::AabbX8& boxes)
        {
            return Simd::Soa::MoveMask(Simd::Intersection::Overlaps(Simd::Soa::Broadcast(bounds), boxes));
        }


        uint32_t FindChild(const Aabb& bounds, const Aabb& entry)
        {
            const Vector3 center = (bounds.min + bounds.max) * 0.5f;
            uint32_t index = 0;
            for (uint32_t axis = 0; axis < 3; ++axis)
            {
                if (entry.min.Data()[axis] >= center.Data()[axis])
                    index |= 1 << axis;
                else if (entry.max.Data()[axis] > center.Data()[axis])
                    return kInvalidIndex;
            }

            return index;
        }
    } // namespace


    Aabb OctreeEntry::GetBounds() const
    {
        FE_Assert(m_block != nullptr);
        return Simd::Soa::Extract(m_block->m_bounds, m_indexInNode);
    }


    Octree::Octree(const Aabb& bounds)
    {
        FE_Assert(bounds.IsValid());
        m_root.m_bounds = bounds;
    }


    Octree::~Octree()
    {
        Release(m_root);
    }


    void Octree::Release(OctreeNode& node)
    {
        if (node.m_children != nullptr)
        {
            for (uint32_t i = 0; i < OctreeNode::kChildCount; ++i)
                Release(node.m_children[i]);

            m_nodePool.Delete(reinterpret_cast<NodeChildrenGroup*>(node.m_children.Get()));
            node.m_children = nullptr;
        }

        while (node.m_blocks)
        {
            OctreeEntryBlock* block = node.m_blocks;
            node.m_blocks = block->m_next;
            m_blockPool.Delete(block);
        }
    }


    void Octree::InsertLocal(OctreeNode& node, OctreeEntry& entry, const Aabb& bounds)
    {
        if (!node.m_blocks || node.m_blocks->m_count == OctreeEntryBlock::kCapacity)
        {
            OctreeEntryBlock* block = m_blockPool.New();
            block->m_next = node.m_blocks;
            node.m_blocks = block;
        }

        OctreeEntryBlock& block = *node.m_blocks;
        entry.m_node = &node;
        entry.m_block = &block;
        entry.m_indexInNode = block.m_count++;
        block.m_entries[entry.m_indexInNode] = &entry;
        Simd::Soa::Insert(block.m_bounds, entry.m_indexInNode, bounds);
        ++node.m_entryCount;
        for (OctreeNode* ancestor = &node; ancestor; ancestor = ancestor->m_parent)
            ++ancestor->m_subtreeEntryCount;
    }


    void Octree::Insert(OctreeNode& node, OctreeEntry& entry, const Aabb& bounds)
    {
        const bool contained = Math::Contains(node.m_bounds, bounds);
        const uint32_t childIndex = contained ? FindChild(node.m_bounds, bounds) : kInvalidIndex;
        const bool canSplit = childIndex != kInvalidIndex && node.m_depth < OctreeNode::kMaxDepth;
        if (node.m_children == nullptr && node.m_entryCount >= OctreeNode::kMaxEntryCount && canSplit)
            Split(node);

        if (node.m_children != nullptr && childIndex != kInvalidIndex)
        {
            Insert(node.m_children[childIndex], entry, bounds);
            return;
        }

        InsertLocal(node, entry, bounds);
    }


    void Octree::Split(OctreeNode& node)
    {
        node.m_children = m_nodePool.New()->m_nodes;
        const Vector3 center = (node.m_bounds.min + node.m_bounds.max) * 0.5f;
        for (uint32_t i = 0; i < OctreeNode::kChildCount; ++i)
        {
            OctreeNode& child = node.m_children[i];
            child.m_parent = &node;
            child.m_depth = node.m_depth + 1;
            child.m_bounds = Aabb(Math::Blend(node.m_bounds.min, center, i), Math::Blend(center, node.m_bounds.max, i));
        }

        OctreeEntryBlock* block = node.m_blocks;
        node.m_blocks = nullptr;
        for (OctreeNode* ancestor = &node; ancestor; ancestor = ancestor->m_parent)
            ancestor->m_subtreeEntryCount -= node.m_entryCount;

        node.m_entryCount = 0;
        while (block)
        {
            for (uint32_t i = 0; i < block->m_count; ++i)
                Insert(node, *block->m_entries[i], Simd::Soa::Extract(block->m_bounds, i));

            OctreeEntryBlock* next = block->m_next;
            m_blockPool.Delete(block);
            block = next;
        }
    }


    void Octree::RemoveLocal(OctreeEntry& entry)
    {
        OctreeNode& node = *entry.m_node;
        OctreeEntryBlock* head = node.m_blocks;
        OctreeEntry* moved = head->m_entries[--head->m_count];
        entry.m_block->m_entries[entry.m_indexInNode] = moved;
        Simd::Soa::Insert(entry.m_block->m_bounds, entry.m_indexInNode, Simd::Soa::Extract(head->m_bounds, head->m_count));
        moved->m_block = entry.m_block;
        moved->m_indexInNode = entry.m_indexInNode;
        entry.m_node = nullptr;
        entry.m_block = nullptr;
        entry.m_indexInNode = kInvalidIndex;
        --node.m_entryCount;
        for (OctreeNode* ancestor = &node; ancestor; ancestor = ancestor->m_parent)
            --ancestor->m_subtreeEntryCount;

        if (head->m_count == 0)
        {
            node.m_blocks = head->m_next;
            m_blockPool.Delete(head);
        }
    }


    void Octree::MergeAncestors(OctreeNode* node)
    {
        while (node)
        {
            if (node->m_children != nullptr)
            {
                uint32_t count = node->m_entryCount;
                for (uint32_t i = 0; i < OctreeNode::kChildCount; ++i)
                {
                    const OctreeNode& child = node->m_children[i];
                    count += child.m_entryCount;
                    if (child.m_children != nullptr)
                        count += OctreeNode::kMaxMergeEntryCount + 1;
                }

                if (count <= OctreeNode::kMaxMergeEntryCount)
                {
                    for (uint32_t i = 0; i < OctreeNode::kChildCount; ++i)
                    {
                        OctreeNode& child = node->m_children[i];
                        for (OctreeNode* ancestor = node; ancestor; ancestor = ancestor->m_parent)
                            ancestor->m_subtreeEntryCount -= child.m_entryCount;

                        for (OctreeEntryBlock* block = child.m_blocks; block; block = block->m_next)
                        {
                            for (uint32_t lane = 0; lane < block->m_count; ++lane)
                                InsertLocal(*node, *block->m_entries[lane], Simd::Soa::Extract(block->m_bounds, lane));
                        }

                        Release(child);
                    }

                    m_nodePool.Delete(reinterpret_cast<NodeChildrenGroup*>(node->m_children.Get()));
                    node->m_children = nullptr;
                }
            }

            node = node->m_parent;
        }
    }


    void Octree::InsertOrUpdate(OctreeEntry& entry, const Aabb& bounds)
    {
        FE_Assert(bounds.IsValid());
        if (entry.m_node)
        {
            OctreeNode* previous = entry.m_node;
            const bool fitsNode = Math::Contains(previous->m_bounds, bounds);
            const bool staysLocal = previous->m_children == nullptr || FindChild(previous->m_bounds, bounds) == kInvalidIndex;
            if (fitsNode && staysLocal)
            {
                Simd::Soa::Insert(entry.m_block->m_bounds, entry.m_indexInNode, bounds);
                return;
            }

            RemoveLocal(entry);
            Insert(m_root, entry, bounds);
            MergeAncestors(previous);
            return;
        }

        Insert(m_root, entry, bounds);
    }


    void Octree::Remove(OctreeEntry& entry)
    {
        if (!entry.m_node)
            return;

        OctreeNode* previous = entry.m_node;
        RemoveLocal(entry);
        MergeAncestors(previous);
    }


    template<class TBounds>
    void Octree::TraverseBlocks(const OctreeNode& node, const TBounds& bounds, const OctreeTraverseCallback& callback,
                                const OctreeEntryBlock* first, const OctreeEntryBlock* stop) const
    {
        for (const OctreeEntryBlock* block = first ? first : node.m_blocks.Get(); block != stop; block = block->m_next)
        {
            uint32_t mask = OverlapMask(bounds, block->m_bounds) & ((1u << block->m_count) - 1);
            OctreeEntry* visible[OctreeEntryBlock::kCapacity];
            uint32_t count = 0;
            while (mask)
            {
                const uint32_t lane = std::countr_zero(mask);
                visible[count++] = block->m_entries[lane];
                mask &= mask - 1;
            }

            if (count)
                callback(node.m_bounds, { visible, count });
        }
    }


    template<class TBounds>
    void Octree::TraverseNode(const OctreeNode& node, const TBounds& bounds, const OctreeTraverseCallback& callback) const
    {
        TraverseBlocks(node, bounds, callback);
        if (node.m_children == nullptr)
            return;

        for (uint32_t i = 0; i < OctreeNode::kChildCount; ++i)
        {
            const OctreeNode& child = node.m_children[i];
            if (Overlaps(bounds, child.m_bounds))
                TraverseNode(child, bounds, callback);
        }
    }


    void Octree::Traverse(const Aabb& bounds, const OctreeTraverseCallback& callback) const
    {
        TraverseNode(m_root, bounds, callback);
    }


    void Octree::Traverse(const Frustum& bounds, const OctreeTraverseCallback& callback) const
    {
        TraverseNode(m_root, bounds, callback);
    }


    void Octree::TraverseParallel(const Frustum& bounds, const OctreeParallelCallback& callback) const
    {
        FE_PROFILER_ZONE();

        struct Task final
        {
            const OctreeNode* m_node;
            bool m_recursive;
            const OctreeEntryBlock* m_first = nullptr;
            const OctreeEntryBlock* m_stop = nullptr;
            uint32_t m_count = 0;
        };

        constexpr uint32_t kEntriesPerBatch = 64;
        festd::fixed_vector<Task, kMaxTraversalTasks> tasks;
        tasks.push_back({ &m_root, true });
        for (uint32_t i = 0; i < tasks.size(); ++i)
        {
            Task& task = tasks[i];
            const OctreeNode& node = *task.m_node;
            task.m_count = task.m_recursive ? node.m_subtreeEntryCount : node.m_entryCount;
            if (!task.m_recursive || node.m_children == nullptr || node.m_subtreeEntryCount <= kEntriesPerBatch)
                continue;

            if (tasks.size() + OctreeNode::kChildCount > kMaxTraversalTasks)
                break;

            task.m_recursive = false;
            task.m_count = node.m_entryCount;
            for (uint32_t childIndex = 0; childIndex < OctreeNode::kChildCount; ++childIndex)
            {
                const OctreeNode& child = node.m_children[childIndex];
                if ((child.m_entryCount || child.m_children != nullptr) && bounds.Overlaps(child.m_bounds))
                    tasks.push_back({ &child, true });
            }
        }

        for (Task& task : tasks)
            task.m_count = task.m_recursive ? task.m_node->m_subtreeEntryCount : task.m_node->m_entryCount;

        // Split large local spill lists as well: crossing a split plane must not serialize a dense scene.
        const uint32_t frontierCount = tasks.size();
        for (uint32_t i = 0; i < frontierCount; ++i)
        {
            Task& task = tasks[i];
            const OctreeNode& node = *task.m_node;
            if (task.m_recursive && node.m_children != nullptr)
                continue;

            const uint32_t blockCount = Math::CeilDivide(node.m_entryCount, OctreeEntryBlock::kCapacity);
            const uint32_t chunks = Math::Min(kMaxTraversalTasks - tasks.size() + 1, Math::CeilDivide(blockCount, 8u));
            if (chunks <= 1)
                continue;

            const uint32_t blocksPerChunk = Math::CeilDivide(blockCount, chunks);
            const OctreeEntryBlock* first = node.m_blocks;
            task.m_recursive = false;
            for (uint32_t chunk = 0; chunk < chunks && first; ++chunk)
            {
                const OctreeEntryBlock* stop = first;
                uint32_t count = 0;
                for (uint32_t block = 0; block < blocksPerChunk && stop; ++block)
                {
                    count += stop->m_count;
                    stop = stop->m_next;
                }

                if (chunk == 0)
                {
                    task.m_first = first;
                    task.m_stop = stop;
                    task.m_count = count;
                }
                else
                {
                    tasks.push_back({ &node, false, first, stop, count });
                }

                first = stop;
            }
        }

        for (uint32_t i = tasks.size(); i > 0; --i)
        {
            const Task& task = tasks[i - 1];
            if (task.m_count == 0)
                tasks.erase(tasks.begin() + i - 1);
        }

        struct Batch final
        {
            uint32_t m_first;
            uint32_t m_end;
            uint32_t m_count;
        };

        festd::fixed_vector<Batch, kMaxTraversalTasks> batches;
        uint32_t first = 0;
        uint32_t count = 0;
        for (uint32_t i = 0; i < tasks.size(); ++i)
        {
            count += tasks[i].m_count;
            if (count >= kEntriesPerBatch || i + 1 == tasks.size())
            {
                batches.push_back({ first, i + 1, count });
                first = i + 1;
                count = 0;
            }
        }

        const auto execute = [&](const uint32_t index) {
            FE_PROFILER_ZONE_NAMED("OctreeTraversalTask");

            const Batch& batch = batches[index];
            ZoneTextF("Batch %u | %u subtrees/ranges | %u entries", index, batch.m_end - batch.m_first, batch.m_count);
            const OctreeTraverseCallback collect = [&](const Aabb&, const festd::span<OctreeEntry* const> entries) {
                callback(index, entries);
            };
            for (uint32_t i = batch.m_first; i < batch.m_end; ++i)
            {
                const Task& task = tasks[i];
                if (task.m_recursive)
                    TraverseNode(*task.m_node, bounds, collect);
                else
                    TraverseBlocks(*task.m_node, bounds, collect, task.m_first, task.m_stop);
            }
        };

        if (batches.empty())
            return;

        if (batches.size() == 1)
        {
            execute(0);
            return;
        }

        const Rc<WaitGroup> completion = WaitGroup::Create();
        for (uint32_t i = 1; i < batches.size(); ++i)
        {
            completion->Add(1);
            Jobs::DispatchForeground(
                [&, i] {
                    execute(i);
                },
                completion.Get());
        }

        execute(0);
        completion->Signal();
        completion->Wait();
    }
} // namespace FE
