#include <Core/Jobs/Jobs.h>
#include <Framework/Entities/EntityCommandListInternal.h>
#include <Framework/Entities/EntityWorld.h>
#include <festd/bit_vector.h>
#include <gtest/gtest.h>

using namespace FE;
using namespace FE::Framework;
namespace EntityInternal = FE::Framework::Internal;

namespace
{
    constexpr uint32_t kIDCount = 1u << 16;
    const Uuid kEntityUuid("08E1F428-DC13-4F4B-AE11-1339C76284A5");
} // namespace


TEST(EntityCommandListIDs, ExhaustionSharingAndReusePreserveReservedZero)
{
    festd::vector<uint16_t> ids;
    ids.reserve(UINT16_MAX);
    auto cleanup = festd::defer([&] {
        for (const uint16_t id : ids)
            EntityInternal::ReleaseCommandListID(id);
    });

    for (uint32_t pass = 0; pass < 2; ++pass)
    {
        festd::fixed_bit_vector<kIDCount> used;
        used.resize(kIDCount, false);
        for (uint32_t i = 0; i < UINT16_MAX; ++i)
        {
            const uint16_t id = EntityInternal::AllocateCommandListID();
            ids.push_back(id);
            EXPECT_NE(id, 0);
            EXPECT_FALSE(used.test(id));
            used.set(id);
        }

        EXPECT_DEATH(EntityInternal::AllocateCommandListID(), "");

        const uint16_t shared = ids.back();
        EXPECT_EQ(EntityInternal::ShareCommandListID(shared), shared);
        EntityInternal::ReleaseCommandListID(shared);
        EXPECT_DEATH(EntityInternal::AllocateCommandListID(), "");

        EntityInternal::ReleaseCommandListID(shared);
        ids.back() = EntityInternal::AllocateCommandListID();
        EXPECT_EQ(ids.back(), shared);

        for (const uint16_t id : ids)
            EntityInternal::ReleaseCommandListID(id);

        ids.clear();
    }
}


TEST(EntityCommandListIDs, ConcurrentAllocationAndReleaseKeepLiveIdentitiesUnique)
{
    constexpr uint32_t kWorkers = 8;
    constexpr uint32_t kListsPerWorker = 256;
    festd::vector<uint16_t> ids(kWorkers * kListsPerWorker);
    const Rc<WaitGroup> allocated = WaitGroup::Create(kWorkers);
    for (uint32_t worker = 0; worker < kWorkers; ++worker)
    {
        Jobs::DispatchForeground(
            [&, worker] {
                for (uint32_t i = 0; i < kListsPerWorker; ++i)
                    ids[worker * kListsPerWorker + i] = EntityInternal::AllocateCommandListID();
            },
            allocated.Get());
    }
    allocated->Wait();

    festd::fixed_bit_vector<kIDCount> used;
    used.resize(kIDCount, false);
    for (const uint16_t id : ids)
    {
        EXPECT_NE(id, 0);
        EXPECT_FALSE(used.test(id));
        used.set(id);
    }

    const Rc<WaitGroup> released = WaitGroup::Create(kWorkers);
    for (uint32_t worker = 0; worker < kWorkers; ++worker)
    {
        Jobs::DispatchForeground(
            [&, worker] {
                for (uint32_t i = 0; i < kListsPerWorker; ++i)
                    EntityInternal::ReleaseCommandListID(ids[worker * kListsPerWorker + i]);
            },
            released.Get());
    }
    released->Wait();
}


TEST(EntityCommandLists, GlobalIdentitiesRejectTokensFromOtherWorlds)
{
    EntityWorld firstWorld, secondWorld;
    auto& firstRegistry = firstWorld.CreateRegistry();
    auto& secondRegistry = secondWorld.CreateRegistry();
    EntityCommandList first(firstWorld), second(secondWorld);
    const auto firstToken = first.CreateEntity(firstRegistry, {}, kEntityUuid);
    const auto secondToken = second.CreateEntity(secondRegistry, {}, kEntityUuid);
    EXPECT_NE(firstToken.m_list, secondToken.m_list);
    EXPECT_DEATH(second.Rename(firstToken, "invalid"), "");
}


TEST(EntityCommandLists, MovesTransferIdentityAndReleaseOverwrittenLists)
{
    EntityWorld world;
    auto& registry = world.CreateRegistry();
    EntityCommandList first(world), second(world);
    const auto firstToken = first.CreateEntity(registry, {}, kEntityUuid);
    const auto secondToken = second.CreateEntity(registry, {}, kEntityUuid);
    second = std::move(first);
    EntityCommandList moved(std::move(second));
    moved.Rename(firstToken, "moved");

    EntityCommandList replacement(world);
    const auto replacementToken = replacement.CreateEntity(registry, {}, kEntityUuid);
    EXPECT_EQ(replacementToken.m_list, secondToken.m_list);
    EXPECT_NE(replacementToken.m_list, firstToken.m_list);
}


TEST(EntityCommandLists, SplitSubmissionsRetainIdentityUntilBothFragmentsAreDestroyed)
{
    EntityWorld world;
    auto& registry = world.CreateRegistry();
    EntityCommandList initial(world);
    initial.CreateEntity(registry, {}, kEntityUuid);
    world.Submit(std::move(initial));
    ASSERT_TRUE(world.CommitBootstrap());
    const EntityID root = world.Find(kEntityUuid)->GetID();
    const Uuid childUuid = Uuid::Random();

    for (bool cancelDeferred : { false, true })
    {
        auto& childRegistry = world.CreateRegistry();
        world.BeginUpdate();
        EntityCommandList commands(world);
        const auto child = commands.CreateEntity(childRegistry, {}, childUuid);
        commands.Rename(root, "updated");
        world.Submit(std::move(commands));
        if (cancelDeferred)
            world.RemoveRegistry(childRegistry);
        else
            ASSERT_TRUE(world.Commit());

        EntityCommandList probe(world);
        const auto probeToken = probe.CreateEntity(registry, {}, kEntityUuid);
        EXPECT_NE(probeToken.m_list, child.m_list);

        if (cancelDeferred)
        {
            ASSERT_TRUE(world.Commit());
        }
        else
        {
            ASSERT_TRUE(world.EndUpdate());
            world.BeginUpdate();
            ASSERT_NE(world.Find(childUuid), nullptr);
        }

        EntityCommandList recycled(world);
        const auto recycledToken = recycled.CreateEntity(registry, {}, kEntityUuid);
        EXPECT_EQ(recycledToken.m_list, child.m_list);
        EXPECT_EQ(world.Find(root)->GetName(), Env::Name("updated"));
        ASSERT_TRUE(world.EndUpdate());
        if (!cancelDeferred)
            world.RemoveRegistry(childRegistry);
    }
}


TEST(EntityCommandLists, CreationIndicesStopBeforeTheInvalidSentinel)
{
    EntityWorld world;
    auto& registry = world.CreateRegistry();
    EntityCommandList commands(world);
    EntityToken last;
    for (uint32_t i = 0; i < UINT16_MAX; ++i)
        last = commands.CreateEntity(registry, {}, kEntityUuid);

    EXPECT_NE(last.m_list, 0);
    EXPECT_EQ(last.m_index, UINT16_MAX - 1);
    EXPECT_EQ(EntityToken{}.m_index, UINT16_MAX);
    EXPECT_DEATH(commands.CreateEntity(registry, {}, kEntityUuid), "");
}
