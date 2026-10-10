#include <Core/Jobs/Jobs.h>
#include <Framework/Entities/EntityRegistryInternal.h>
#include <Framework/Entities/EntityWorld.h>
#include <festd/bit_vector.h>
#include <gtest/gtest.h>

using namespace FE;
using namespace FE::Framework;
namespace EntityInternal = FE::Framework::Internal;


TEST(EntityRegistryIDs, GrowthAndRetainedReferencesPreserveUniqueness)
{
    EntityInternal::RegistryIDPool pool;
    constexpr uint32_t kCount = 256;
    festd::vector<uint32_t> ids;
    festd::bit_vector used;
    used.resize(kCount + 2, false);
    for (uint32_t i = 0; i < kCount; ++i)
    {
        const uint32_t id = pool.Allocate();
        ASSERT_NE(id, 0);
        ASSERT_LT(id, used.size());
        EXPECT_FALSE(used.test(id));
        used.set(id);
        ids.push_back(id);
    }

    const uint32_t retained = ids.front();
    pool.Retain(retained);
    for (const uint32_t id : ids)
        pool.Release(id);

    ids.clear();
    for (uint32_t i = 0; i < kCount; ++i)
    {
        const uint32_t id = pool.Allocate();
        EXPECT_NE(id, retained);
        ids.push_back(id);
    }

    pool.Release(retained);
    const uint32_t recycled = pool.Allocate();
    EXPECT_EQ(recycled, retained);
    pool.Release(recycled);
    for (const uint32_t id : ids)
        pool.Release(id);
}


TEST(EntityRegistryIDs, ConcurrentAllocationAndReleaseKeepLiveIdentitiesUnique)
{
    EntityInternal::RegistryIDPool pool;
    constexpr uint32_t kWorkers = 8;
    constexpr uint32_t kRegistriesPerWorker = 64;
    festd::vector<uint32_t> ids(kWorkers * kRegistriesPerWorker);
    const Rc<WaitGroup> allocated = WaitGroup::Create(kWorkers);
    for (uint32_t worker = 0; worker < kWorkers; ++worker)
    {
        Jobs::DispatchForeground(
            [&, worker] {
                for (uint32_t i = 0; i < kRegistriesPerWorker; ++i)
                    ids[worker * kRegistriesPerWorker + i] = pool.Allocate();
            },
            allocated.Get());
    }
    allocated->Wait();

    festd::bit_vector used;
    used.resize(ids.size() + 1, false);
    for (const uint32_t id : ids)
    {
        ASSERT_NE(id, 0);
        ASSERT_LT(id, used.size());
        EXPECT_FALSE(used.test(id));
        used.set(id);
    }

    const Rc<WaitGroup> released = WaitGroup::Create(kWorkers);
    for (uint32_t worker = 0; worker < kWorkers; ++worker)
    {
        Jobs::DispatchForeground(
            [&, worker] {
                for (uint32_t i = 0; i < kRegistriesPerWorker; ++i)
                    pool.Release(ids[worker * kRegistriesPerWorker + i]);
            },
            released.Get());
    }
    released->Wait();
}


TEST(EntityRegistries, RemovedRegistriesKeepIDsReservedUntilRecordedCommandsAreConsumed)
{
    for (const bool unload : { false, true })
    {
        EntityWorld world;
        auto& registry = world.CreateRegistry();
        const uint32_t oldID = registry.GetID();
        EntityCommandList commands(world);
        if (unload)
            commands.UnloadRegistry(registry);
        else
            commands.CreateEntity(registry);

        world.RemoveRegistry(registry);
        auto& replacement = world.CreateRegistry();
        EXPECT_NE(replacement.GetID(), oldID);
        world.Submit(std::move(commands));
        EXPECT_FALSE(world.CommitBootstrap());
        EXPECT_EQ(world.GetEntityCount(), 0);

        auto& recycled = world.CreateRegistry();
        EXPECT_EQ(recycled.GetID(), oldID);
    }
}


TEST(EntityRegistries, WorldsAllocateAndReuseIDsIndependently)
{
    EntityWorld firstWorld, secondWorld;
    auto& firstRegistry = firstWorld.CreateRegistry();
    auto& secondRegistry = secondWorld.CreateRegistry();
    const uint32_t id = firstRegistry.GetID();
    EXPECT_EQ(secondRegistry.GetID(), id);

    {
        EntityCommandList commands(firstWorld);
        commands.CreateEntity(firstRegistry);
        firstWorld.RemoveRegistry(firstRegistry);
        EXPECT_NE(firstWorld.CreateRegistry().GetID(), id);

        secondWorld.RemoveRegistry(secondRegistry);
        EXPECT_EQ(secondWorld.CreateRegistry().GetID(), id);
    }

    EXPECT_EQ(firstWorld.CreateRegistry().GetID(), id);
}
