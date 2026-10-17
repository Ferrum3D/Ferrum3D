#include <Core/Math/Octree.h>
#include <Core/SIMD/Intersection.h>
#include <bit>
#include <festd/vector.h>
#include <gtest/gtest.h>

using namespace FE;

namespace
{
    bool ClipCornerReference(const Matrix4x4& matrix, const Aabb& box)
    {
        bool outside[6] = { true, true, true, true, true, true };
        for (uint32_t i = 0; i < 8; ++i)
        {
            const Vector4 clip = Vector4(Math::Blend(box.min, box.max, i), 1.0f) * matrix;
            outside[0] &= clip.x < -clip.w;
            outside[1] &= clip.x > clip.w;
            outside[2] &= clip.y < -clip.w;
            outside[3] &= clip.y > clip.w;
            outside[4] &= clip.z < 0.0f;
            outside[5] &= clip.z > clip.w;
        }

        for (const bool rejected : outside)
        {
            if (rejected)
                return false;
        }

        return true;
    }


    void CheckTree(const Octree& tree, const festd::vector<OctreeEntry>& entries, const festd::vector<Aabb>& bounds,
                   const Frustum& frustum)
    {
        festd::vector<uint32_t> serial(entries.size(), 0);
        tree.Traverse(frustum, [&](const Aabb&, const festd::span<OctreeEntry* const> found) {
            for (const OctreeEntry* entry : found)
                ++serial[entry->m_userIndex];
        });

        std::atomic<uint32_t> parallel[1024]{};
        ASSERT_LE(entries.size(), festd::size(parallel));
        tree.TraverseParallel(frustum, [&](const uint32_t task, const festd::span<OctreeEntry* const> found) {
            EXPECT_LT(task, Octree::kMaxTraversalTasks);
            for (const OctreeEntry* entry : found)
                ++parallel[entry->m_userIndex];
        });

        for (uint32_t i = 0; i < entries.size(); ++i)
        {
            if (entries[i].m_node)
                EXPECT_TRUE(Math::CmpEqual(entries[i].GetBounds(), bounds[i])) << i;

            const uint32_t expected = entries[i].m_node && frustum.Overlaps(bounds[i]) ? 1 : 0;
            EXPECT_EQ(serial[i], expected) << i;
            EXPECT_EQ(parallel[i].load(), expected) << i;
        }
    }
} // namespace


TEST(Frustum, ScalarAndSimdMatchClipSpaceWithReversedAndInfiniteDepth)
{
    Matrix4x4 infinite = Matrix4x4::Projection(1.1f, 1.7f, 0.1f, 1000.0f);
    infinite.m_22 = 0.0f;
    infinite.m_32 = 0.1f;
    const Matrix4x4 matrices[] = { Matrix4x4::kIdentity,
                                   Matrix4x4::Projection(1.1f, 1.7f, 0.1f, 1000.0f),
                                   infinite,
                                   Matrix4x4::Rotation(Quaternion::RotationY(0.6f))
                                       * Matrix4x4::Projection(0.8f, 1.3f, 0.2f, 50.0f) };
    for (const Matrix4x4& matrix : matrices)
    {
        const Frustum frustum(matrix);
        for (uint32_t group = 0; group < 128; ++group)
        {
            Simd::Soa::AabbX8 boxes{};
            uint32_t expected = 0;
            for (uint32_t lane = 0; lane < 8; ++lane)
            {
                const uint32_t i = group * 8 + lane;
                const Vector3 position(float(i % 13) * 1.37f - 7.1f, float(i % 17) * 0.83f - 6.3f, float(i % 29) * 2.17f - 5.3f);
                const Aabb box(position - Vector3(0.23f), position + Vector3(0.23f));
                const bool visible = ClipCornerReference(matrix, box);
                EXPECT_EQ(frustum.Overlaps(box), visible);
                expected |= uint32_t(visible) << lane;
                Simd::Soa::Insert(boxes, lane, box);
            }

            EXPECT_EQ(Simd::Soa::MoveMask(Simd::Intersection::Overlaps(frustum, boxes)), expected);
        }
    }
}


TEST(Frustum, IncludesPlaneContactsAndRejectsBoxesBehindCamera)
{
    const Frustum clip(Matrix4x4::kIdentity);
    EXPECT_TRUE(clip.Overlaps(Aabb(Vector3(-1, -1, 0), Vector3(1, 1, 1))));
    EXPECT_TRUE(clip.Overlaps(Aabb(Vector3(1, 0, 0), Vector3(2, 1, 1))));
    EXPECT_FALSE(clip.Overlaps(Aabb(Vector3(2, 0, 0), Vector3(3, 1, 1))));
    const Frustum perspective(Matrix4x4::Projection(1.0f, 1.0f, 0.1f, 100.0f));
    EXPECT_FALSE(perspective.Overlaps(Aabb(Vector3(-1, -1, -4), Vector3(1, 1, -2))));
}


TEST(Octree, ParallelAndSerialMatchBruteForceAfterMovementRemovalAndMerges)
{
    static_assert(sizeof(OctreeNode) == 64);
    static_assert(sizeof(OctreeEntry) == 24);
    Octree tree(Aabb(Vector3(-64), Vector3(64)));
    festd::vector<OctreeEntry> entries(1024);
    festd::vector<Aabb> bounds(entries.size());
    for (uint32_t i = 0; i < entries.size(); ++i)
    {
        OctreeEntry& entry = entries[i];
        const Vector3 position(float(i % 16) * 7.3f - 55.1f, float((i / 16) % 8) * 7.3f - 25.1f, float(i / 128) * 7.3f + 2.3f);
        bounds[i] = Aabb(position, position + Vector3(0.6f));
        entry.m_userIndex = i;
        tree.InsertOrUpdate(entry, bounds[i]);
    }

    const Frustum frustum(Matrix4x4::Projection(1.2f, 1.7f, 0.1f, 200.0f));
    CheckTree(tree, entries, bounds, frustum);
    for (uint32_t pass = 0; pass < 4; ++pass)
    {
        for (uint32_t i = 0; i < entries.size(); ++i)
        {
            if ((i + pass) % 3 == 0)
            {
                tree.Remove(entries[i]);
            }
            else
            {
                const Vector3 position(float(i % 19) * 3.7f - 32.1f, float(i % 7) - 3.3f, float(i % 101) + 0.7f);
                bounds[i] = Aabb(position, position + Vector3(0.3f));
                tree.InsertOrUpdate(entries[i], bounds[i]);
            }
        }

        CheckTree(tree, entries, bounds, frustum);
    }

    for (uint32_t i = 8; i < entries.size(); ++i)
        tree.Remove(entries[i]);

    CheckTree(tree, entries, bounds, frustum);
    for (OctreeEntry& entry : entries)
        tree.Remove(entry);
}


TEST(Octree, CoincidentPointsAndRootOverflowRemainQueryable)
{
    Octree tree(Aabb(Vector3(-1), Vector3(1)));
    festd::vector<OctreeEntry> entries(1024);
    festd::vector<Aabb> bounds(entries.size());
    for (uint32_t i = 0; i < entries.size(); ++i)
    {
        bounds[i] = Aabb(Vector3(0.5f), Vector3(0.5f));
        entries[i].m_userIndex = i;
        tree.InsertOrUpdate(entries[i], bounds[i]);
    }

    CheckTree(tree, entries, bounds, Frustum(Matrix4x4::kIdentity));
    bounds[0] = Aabb(Vector3(50, 0, 0.5f), Vector3(51, 1, 0.7f));
    tree.InsertOrUpdate(entries[0], bounds[0]);
    uint32_t found = 0;
    tree.Traverse(bounds[0], [&](const Aabb&, const festd::span<OctreeEntry* const> batch) {
        found += batch.size();
    });
    EXPECT_EQ(found, 1);
    bounds[0] = Aabb(Vector3(0.4f), Vector3(0.6f));
    tree.InsertOrUpdate(entries[0], bounds[0]);
    CheckTree(tree, entries, bounds, Frustum(Matrix4x4::kIdentity));
    for (OctreeEntry& entry : entries)
        tree.Remove(entry);
}


TEST(Octree, ParallelizesEntriesThatCannotBeSubdivided)
{
    Octree tree(Aabb(Vector3(-1), Vector3(1)));
    festd::vector<OctreeEntry> entries(512);
    const Aabb bounds(Vector3(-0.2f, -0.2f, 0.2f), Vector3(0.2f, 0.2f, 0.8f));
    for (OctreeEntry& entry : entries)
        tree.InsertOrUpdate(entry, bounds);

    std::atomic<uint64_t> tasks = 0;
    std::atomic<uint32_t> found = 0;
    tree.TraverseParallel(Frustum(Matrix4x4::kIdentity), [&](const uint32_t task, const festd::span<OctreeEntry* const> batch) {
        tasks.fetch_or(uint64_t(1) << task);
        found.fetch_add(batch.size());
    });
    EXPECT_EQ(found.load(), entries.size());
    EXPECT_GT(std::popcount(tasks.load()), 1);
    for (OctreeEntry& entry : entries)
        tree.Remove(entry);
}


TEST(Octree, FlatSceneCombinesSmallSubtreesIntoFullBatches)
{
    Octree tree(Aabb(Vector3(-1), Vector3(1)));
    festd::vector<OctreeEntry> entries(512);
    for (uint32_t i = 0; i < entries.size(); ++i)
    {
        const Vector3 position(float(i % 32) / 32.0f - 0.48f, 0.03f, float(i / 32) / 32.0f + 0.01f);
        tree.InsertOrUpdate(entries[i], Aabb(position, position + Vector3(0.001f)));
    }

    std::atomic<uint32_t> counts[Octree::kMaxTraversalTasks]{};
    tree.TraverseParallel(Frustum(Matrix4x4::kIdentity), [&](const uint32_t task, const festd::span<OctreeEntry* const> batch) {
        counts[task].fetch_add(batch.size());
    });
    uint32_t batches = 0;
    uint32_t found = 0;
    for (const auto& count : counts)
    {
        const uint32_t value = count.load();
        batches += value != 0;
        found += value;
    }

    EXPECT_EQ(found, entries.size());
    EXPECT_GT(batches, 1);
    EXPECT_LE(batches, 8);
    for (OctreeEntry& entry : entries)
        tree.Remove(entry);
}
