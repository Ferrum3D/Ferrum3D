#include <EntityTestTypes.h>
#include <Framework/Entities/EntityWorldInstance.h>
#include <Framework/Entities/WorldTimeService.h>
#include <gtest/gtest.h>

using namespace FE;
using namespace FE::Framework;
using namespace FE::Framework::Tests;

namespace
{
    struct DeltaProbe final : WorldSystem
    {
        float m_simulation = -1;
        float m_tools = -1;

        void Update(EntityUpdateContext& context) override
        {
            m_simulation = context.m_deltaSeconds;
            m_tools = context.m_unpausedDeltaSeconds;
        }
    };
} // namespace


TEST(EditorRuntime, PausedWorldStillCollectsUpdatesWithToolDelta)
{
    WorldTimeService time;
    DeltaProbe probe;
    EntityWorld world;
    world.AddService(time);
    world.AddSystem(probe);
    time.SetPaused(true);
    world.BeginUpdate();
    ASSERT_TRUE(world.ExecuteSchedule());
    ASSERT_TRUE(world.EndUpdate());
    EXPECT_FLOAT_EQ(probe.m_simulation, 0);
    EXPECT_FLOAT_EQ(probe.m_tools, time.GetUnpausedDeltaSeconds());
    EXPECT_GE(probe.m_tools, 0);
}


TEST(EditorRuntime, PauseFreezesSimulationTimeAndKeepsToolTime)
{
    EntityWorld world;
    WorldTimeService time;
    time.Init(world);
    time.Advance(0.25f);
    EXPECT_FLOAT_EQ(time.GetDeltaSeconds(), 0.25f);
    EXPECT_DOUBLE_EQ(time.GetElapsedSeconds(), 0.25);

    time.SetPaused(true);
    EXPECT_FLOAT_EQ(time.GetDeltaSeconds(), 0);
    time.Advance(0.5f);
    EXPECT_FLOAT_EQ(time.GetDeltaSeconds(), 0);
    EXPECT_FLOAT_EQ(time.GetUnpausedDeltaSeconds(), 0.5f);
    EXPECT_DOUBLE_EQ(time.GetElapsedSeconds(), 0.25);

    time.SetPaused(false);
    time.Advance(0.125f);
    EXPECT_FLOAT_EQ(time.GetDeltaSeconds(), 0.125f);
    EXPECT_DOUBLE_EQ(time.GetElapsedSeconds(), 0.375);
}


TEST(EditorRuntime, TransientRegistryNeverEntersWorldSnapshots)
{
    EntityWorldInstance instance;
    EntityWorldAsset definition;
    definition.m_systems.push_back(Rtti::GetTypeID<SnapshotSystem>());
    ASSERT_TRUE(instance.Load(definition));
    auto& world = instance.GetWorld();
    auto& authored = world.CreateRegistry();
    auto& transient = world.CreateRegistry();
    transient.SetTransient(true);
    const auto authoredUuid = Uuid::Random();
    const auto cameraUuid = Uuid::Random();
    EntityCommandList commands(world);
    const auto entity = commands.CreateEntity(authored, Env::Name("Authored"), authoredUuid);
    ASSERT_TRUE(commands.AddComponent(entity, Number{ 7 }));
    commands.SetActive(entity, true);
    const auto camera = commands.CreateEntity(transient, Env::Name("Editor Camera"), cameraUuid);
    ASSERT_TRUE(commands.AddComponent(camera, Number{ 9 }));
    commands.SetActive(camera, true);
    world.Submit(std::move(commands));
    ASSERT_TRUE(world.CommitBootstrap());
    ASSERT_NE(world.Find(cameraUuid), nullptr);
    EXPECT_EQ(world.GetEntityCount(), 2);
    EXPECT_FALSE(world.GetComponents(*world.Find(authoredUuid)).empty());

    EntityWorldSnapshotAsset snapshot;
    ASSERT_TRUE(instance.Capture(snapshot));
    ASSERT_EQ(snapshot.m_registries.size(), 1);
    EXPECT_EQ(snapshot.m_registries[0].m_key, authored.GetKey());
    ASSERT_EQ(snapshot.m_registries[0].m_entities.m_entities.size(), 1);
    EXPECT_EQ(snapshot.m_registries[0].m_entities.m_entities[0].m_uuid, authoredUuid);

    EntityWorldInstance restored;
    ASSERT_TRUE(restored.Restore(snapshot));
    ASSERT_TRUE(restored.GetWorld().CommitBootstrap());
    EXPECT_NE(restored.GetWorld().Find(authoredUuid), nullptr);
    EXPECT_EQ(restored.GetWorld().Find(cameraUuid), nullptr);
}


TEST(EditorRuntime, RootTraversalTracksReparentingDestructionAndEarlyExit)
{
    EntityWorld world;
    auto& registry = world.CreateRegistry();
    const Uuid first = Uuid::Random();
    const Uuid second = Uuid::Random();
    const Uuid child = Uuid::Random();
    EntityCommandList commands(world);
    const auto firstToken = commands.CreateEntity(registry, {}, first);
    commands.AddComponent(firstToken, Number{ 1 });
    const auto secondToken = commands.CreateEntity(registry, {}, second);
    commands.AddComponent(secondToken, Number{ 2 });
    const auto childToken = commands.CreateEntity(registry, {}, child);
    commands.AddComponent(childToken, Number{ 3 });
    commands.SetParent(childToken, firstToken, ReparentMode::kPreserveLocal);
    world.Submit(std::move(commands));
    ASSERT_TRUE(world.CommitBootstrap());
    uint32_t roots = 0;
    world.TraverseRoots([&](Entity& entity) {
        EXPECT_NE(entity.GetUuid(), child);
        EXPECT_EQ(entity.GetParent(), nullptr);
        ++roots;
        return true;
    });
    EXPECT_EQ(roots, 2);
    uint32_t visits = 0;
    world.TraverseRoots([&](Entity&) {
        ++visits;
        return false;
    });
    EXPECT_EQ(visits, 1);
    EXPECT_EQ(world.Find(first)->GetFirstChild(), world.Find(child));

    EntityCommandList detach(world);
    detach.SetParent(world.Find(child)->GetID(), {}, ReparentMode::kPreserveLocal);
    detach.Destroy(world.Find(first)->GetID());
    world.Submit(std::move(detach));
    world.BeginUpdate();
    ASSERT_TRUE(world.ExecuteSchedule());
    ASSERT_TRUE(world.EndUpdate());
    roots = 0;
    world.TraverseRoots([&](Entity& entity) {
        EXPECT_NE(entity.GetUuid(), first);
        ++roots;
        return true;
    });
    EXPECT_EQ(roots, 2);
    world.Clear();
    world.TraverseRoots([](Entity&) {
        ADD_FAILURE() << "An empty world has no roots";
        return true;
    });
}
