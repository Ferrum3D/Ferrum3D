#include <EntityTestTypes.h>
#include <Framework/Entities/EntityWorldInstance.h>
#include <gtest/gtest.h>

using namespace FE;
using namespace FE::Framework;
using namespace FE::Framework::Tests;

namespace
{
    EntityWorldAsset MakeWorld()
    {
        EntityWorldAsset asset;
        asset.m_systems.push_back(Rtti::GetTypeID<SnapshotSystem>());
        auto& group = asset.m_registries.emplace_back();
        group.m_key = Uuid::Random();
        auto& root = group.m_entities.m_entities.emplace_back();
        root.m_uuid = Uuid::Random();
        root.m_name = "Root";
        FE_Assert(group.m_entities.CookComponent(root, Number{ 3 }));
        EntityRecord child;
        child.m_uuid = Uuid::Random();
        child.m_parentUuid = root.m_uuid;
        child.m_name = "Child";
        ReferenceComponent reference;
        reference.m_internal.m_uuid = root.m_uuid;
        FE_Assert(group.m_entities.CookComponent(child, reference));
        group.m_entities.m_entities.push_back(std::move(child));
        return asset;
    }
} // namespace


TEST(EntityWorldAssets, ReusableDefinitionCreatesIndependentWorldsAndOwnedSystems)
{
    const auto asset = MakeWorld();
    const auto uuid = asset.m_registries[0].m_entities.m_entities[0].m_uuid;
    {
        EntityWorldInstance first, second;
        ASSERT_TRUE(first.Load(asset));
        ASSERT_TRUE(second.Load(asset));
        ASSERT_TRUE(first.GetWorld().CommitBootstrap());
        ASSERT_TRUE(second.GetWorld().CommitBootstrap());
        EXPECT_EQ(first.GetWorld().GetEntityCount(), 2);
        EXPECT_EQ(SnapshotSystem::s_live, 2);
        EXPECT_NE(first.GetWorld().Find(uuid)->GetID(), second.GetWorld().Find(uuid)->GetID());
        EXPECT_EQ(second.GetWorld().Find(first.GetWorld().Find(uuid)->GetID()), nullptr);
    }
    EXPECT_EQ(SnapshotSystem::s_live, 0);
}


TEST(EntityWorldAssets, SnapshotPreservesConcreteEditsHierarchyReferencesAndInactiveRows)
{
    const auto asset = MakeWorld();
    const auto root = asset.m_registries[0].m_entities.m_entities[0].m_uuid;
    const auto child = asset.m_registries[0].m_entities.m_entities[1].m_uuid;
    EntityWorldSnapshotAsset snapshot;
    EntityID oldId;
    {
        EntityWorldInstance original;
        ASSERT_TRUE(original.Load(asset));
        auto& world = original.GetWorld();
        ASSERT_TRUE(world.CommitBootstrap());
        oldId = world.Find(root)->GetID();
        world.Find(root)->FindComponent<Number>()->m_value = 19;
        EntityCommandList edit(world);
        edit.Rename(world.Find(root)->GetID(), Env::Name("Changed"));
        edit.SetActive(world.Find(child)->GetID(), false);
        world.Submit(std::move(edit));
        EXPECT_FALSE(original.Capture(snapshot));
        EXPECT_EQ(world.GetLastError(), "Snapshot blocked by pending entity commands");
        ASSERT_TRUE(world.CommitBootstrap());
        ASSERT_TRUE(original.Capture(snapshot));
        EXPECT_EQ(snapshot.m_registries[0].m_entities.m_entities[0].m_components.size(), 1);
    }
    EntityWorldInstance restored;
    ASSERT_TRUE(restored.Restore(snapshot));
    auto& world = restored.GetWorld();
    ASSERT_TRUE(world.CommitBootstrap());
    EXPECT_EQ(world.Find(oldId), nullptr);
    EXPECT_EQ(world.Find(root)->GetName(), Env::Name("Changed"));
    EXPECT_EQ(world.Find(root)->FindComponent<const Number>()->m_value, 19);
    ASSERT_NE(world.Find(child, false), nullptr);
    EXPECT_FALSE(world.Find(child, false)->IsActive());
    EXPECT_EQ(world.Find(child, false)->GetParent(), world.Find(root));
    EXPECT_EQ(world.Find(child, false)->FindComponent<const ReferenceComponent>()->m_internal.m_uuid, root);
    EXPECT_NE(world.Find(root)->FindComponent<const Extra>(), nullptr);
}


TEST(EntityWorldAssets, RestoredPlacementDoesNotRespawnDeletedSourceRows)
{
    EntityCollection collection;
    for (uint32_t i = 0; i < 2; ++i)
    {
        EntityRecord record;
        record.m_uuid = Uuid::Random();
        ASSERT_TRUE(collection.CookComponent(record, Number{ static_cast<int32_t>(i) }));
        collection.m_entities.push_back(std::move(record));
    }
    EntityCollectionInstanceAsset placement;
    placement.m_collection = IO::Link<EntityCollection>(Uuid::Random());
    ASSERT_TRUE(placement.UpdateBindings(collection));
    const auto placementId = Uuid::Random();
    const auto registryKey = Uuid::Random();
    const auto deleted = placement.m_bindings[1].m_entityUuid;
    EntityWorldSnapshotAsset snapshot;
    {
        EntityWorld world;
        world.Components().Register<Number>();
        auto& registry = world.CreateRegistry(registryKey);
        const auto operation = world.LoadPlacement(registry, placementId, placement, collection);
        ASSERT_TRUE(world.CommitBootstrap());
        EntityCommandList commands(world);
        commands.Destroy(world.Find(deleted)->GetID());
        world.Submit(std::move(commands));
        ASSERT_TRUE(world.CommitBootstrap());
        ASSERT_TRUE(world.CaptureSnapshot(snapshot));
        EXPECT_EQ(world.GetMaterializationStatus(operation).m_state, MaterializationState::kReady);
    }
    EntityWorld restored;
    restored.Components().Register<Number>();
    ASSERT_TRUE(restored.RestoreSnapshot(snapshot));
    ASSERT_TRUE(restored.CommitBootstrap());
    EXPECT_EQ(restored.GetEntityCount(), 2);
    EXPECT_EQ(restored.Find(deleted, false), nullptr);
    const auto operation = restored.LoadPlacement(*restored.FindRegistry(registryKey), placementId, placement, collection);
    ASSERT_TRUE(restored.CommitBootstrap());
    EXPECT_EQ(restored.GetMaterializationStatus(operation).m_state, MaterializationState::kReady);
    EXPECT_EQ(restored.GetEntityCount(), 2);
    restored.RemoveRegistry(*restored.FindRegistry(registryKey));
    EXPECT_EQ(restored.GetEntityCount(), 0);
}


TEST(EntityWorldAssets, InvalidDefinitionsAndSystemTypesLeaveInstanceEmpty)
{
    auto asset = MakeWorld();
    EntityWorldInstance instance;
    const auto system = asset.m_systems[0];
    for (const auto invalid :
         { Uuid::Random(), Rtti::GetTypeID<Number>(), Rtti::GetTypeID<WorldSystem>(), Rtti::GetTypeID<NonDefaultSystem>() })
    {
        asset.m_systems[0] = invalid;
        EXPECT_FALSE(instance.Load(asset));
        EXPECT_EQ(instance.GetWorld().GetEntityCount(), 0);
        EXPECT_EQ(SnapshotSystem::s_live, 0);
    }

    asset.m_systems = { system, system };
    EXPECT_FALSE(instance.Load(asset));
    EXPECT_EQ(SnapshotSystem::s_live, 0);
    asset.m_systems = { system };
    asset.m_registries[0].m_entities.m_entities[0].m_parentUuid = Uuid::Random();
    EXPECT_FALSE(instance.Load(asset));
    EXPECT_EQ(SnapshotSystem::s_live, 0);
}


TEST(EntityWorldAssets, SnapshotPreservesInactiveSubtreeAndSiblingOrder)
{
    auto asset = MakeWorld();
    auto& collection = asset.m_registries[0].m_entities;
    EntityRecord secondChild;
    secondChild.m_uuid = Uuid::Random();
    secondChild.m_parentUuid = collection.m_entities[0].m_uuid;
    const Uuid sibling = secondChild.m_uuid;
    const Uuid root = secondChild.m_parentUuid;
    const Uuid firstChild = collection.m_entities[1].m_uuid;
    collection.m_entities.push_back(std::move(secondChild));
    EntityWorldInstance instance;
    ASSERT_TRUE(instance.Load(asset));
    auto& world = instance.GetWorld();
    ASSERT_TRUE(world.CommitBootstrap());
    EntityCommandList commands(world);
    commands.SetParent(world.Find(firstChild)->GetID(), {}, ReparentMode::kPreserveLocal);
    world.Submit(std::move(commands));
    ASSERT_TRUE(world.CommitBootstrap());
    EntityCommandList reorder(world);
    reorder.SetParent(world.Find(firstChild)->GetID(), world.Find(root)->GetID(), ReparentMode::kPreserveLocal);
    reorder.SetActive(world.Find(root)->GetID(), false);
    world.Submit(std::move(reorder));
    ASSERT_TRUE(world.CommitBootstrap());
    EntityWorldSnapshotAsset snapshot;
    ASSERT_TRUE(instance.Capture(snapshot));
    EntityWorldInstance restored;
    ASSERT_TRUE(restored.Restore(snapshot));
    ASSERT_TRUE(restored.GetWorld().CommitBootstrap());
    EntityWorldSnapshotAsset savedAgain;
    EXPECT_TRUE(restored.Capture(savedAgain));
    const auto* parent = restored.GetWorld().Find(root, false);
    ASSERT_NE(parent, nullptr);
    EXPECT_FALSE(parent->IsActive());
    const auto* original = world.Find(root, false)->GetFirstChild();
    EXPECT_EQ(parent->GetFirstChild()->GetUuid(), original->GetUuid());
    EXPECT_EQ(parent->GetFirstChild()->GetNextSibling()->GetUuid(), original->GetNextSibling()->GetUuid());
    EXPECT_NE(restored.GetWorld().Find(sibling, false), nullptr);
}


TEST(EntityWorldAssets, RecordedStagesShareAnExplicitApplicationPhase)
{
    EntityWorld world;
    constexpr Phase phase{ 100, "SharedStage" };
    uint32_t visited = 0;
    world.BeginUpdate();
    const auto first = world.RecordStage(phase, [&] {
        visited = 1;
    });
    const Rc<WaitGroup> prerequisites[] = { first };
    world.RecordStage(
        phase,
        [&] {
            EXPECT_EQ(visited, 1);
            visited = 2;
        },
        prerequisites);
    ASSERT_TRUE(world.SchedulePhase(phase));
    ASSERT_TRUE(world.ExecuteSchedule());
    ASSERT_TRUE(world.EndUpdate());
    EXPECT_EQ(visited, 2);
}


TEST(EntityWorldAssets, ReflectedConstructionAdjustsMultipleInheritanceBaseAddress)
{
    auto asset = MakeWorld();
    asset.m_systems[0] = Rtti::GetTypeID<OffsetSnapshotSystem>();
    {
        EntityWorldInstance instance;
        ASSERT_TRUE(instance.Load(asset));
        ASSERT_TRUE(instance.GetWorld().CommitBootstrap());
        EXPECT_EQ(instance.GetWorld().GetEntityCount(), 2);
        EXPECT_EQ(SnapshotSystem::s_live, 1);
        EntityWorldSnapshotAsset snapshot;
        ASSERT_TRUE(instance.Capture(snapshot));
        EXPECT_EQ(snapshot.m_systems, asset.m_systems);
    }

    EXPECT_EQ(SnapshotSystem::s_live, 0);
}


TEST(EntityWorldAssets, ReflectedServicesSurviveEntityTeardownAndSnapshotRestore)
{
    auto asset = MakeWorld();
    asset.m_services.push_back(Rtti::GetTypeID<OffsetSnapshotService>());
    EntityWorldSnapshotAsset snapshot;
    SnapshotService::s_shutdownSawEmptyWorld = false;
    {
        EntityWorldInstance instance;
        ASSERT_TRUE(instance.Load(asset));
        auto& world = instance.GetWorld();
        ASSERT_TRUE(world.CommitBootstrap());
        auto* service = world.FindService<SnapshotService>();
        ASSERT_NE(service, nullptr);
        EXPECT_EQ(SnapshotService::s_live, 1);
        world.BeginUpdate();
        ASSERT_TRUE(world.ExecuteSchedule());
        ASSERT_TRUE(world.EndUpdate());
        EXPECT_EQ(service->m_updates, 1);
        ASSERT_TRUE(instance.Capture(snapshot));
        EXPECT_EQ(snapshot.m_services, asset.m_services);
    }

    EXPECT_TRUE(SnapshotService::s_shutdownSawEmptyWorld);
    EXPECT_EQ(SnapshotService::s_live, 0);
    {
        EntityWorldInstance instance;
        ASSERT_TRUE(instance.Restore(snapshot));
        ASSERT_TRUE(instance.GetWorld().CommitBootstrap());
        EXPECT_NE(instance.GetWorld().FindService<SnapshotService>(), nullptr);
        EXPECT_EQ(instance.GetWorld().GetEntityCount(), 2);
    }

    EXPECT_EQ(SnapshotService::s_live, 0);
}


TEST(EntityWorldAssets, InvalidServiceTypesRollBackAlreadyInitializedServices)
{
    auto asset = MakeWorld();
    const auto id = Rtti::GetTypeID<SnapshotService>();
    for (const auto invalid : { Uuid::Random(), Rtti::GetTypeID<Number>(), Rtti::GetTypeID<SnapshotSystem>(), id })
    {
        asset.m_services = { id, invalid };
        EntityWorldInstance instance;
        EXPECT_FALSE(instance.Load(asset));
        EXPECT_EQ(SnapshotService::s_live, 0);
        EXPECT_EQ(instance.GetWorld().FindService<SnapshotService>(), nullptr);
        EXPECT_EQ(instance.GetWorld().GetEntityCount(), 0);
    }
}
