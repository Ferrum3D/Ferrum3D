#include <AssetBuilder/AssetFile.h>
#include <AssetBuilder/AssetPipeline.h>
#include <Core/IO/Artifact.h>
#include <Core/Strings/Format.h>
#include <Core/Threading/Thread.h>
#include <EntityTestTypes.h>
#include <Framework/Entities/EntityWorldAsset.h>
#include <gtest/gtest.h>

using namespace FE;
using namespace FE::Framework;
using namespace FE::Framework::Tests;

namespace
{
    struct CollectionAssetsIntegration : testing::Test
    {
        IO::Path m_directory;
        void SetUp() override
        {
            m_directory = IO::Path(FE_ENTITY_ASSET_TEST_OUTPUT) / Fmt::Format("{}", Uuid::Random());
        }


        template<class T>
        IO::AssetID BuildDefinition(const char* name, const T& value)
        {
            const IO::Path path = m_directory / name;
            EXPECT_TRUE(AssetBuilder::ImportAsset({ {}, path, Rtti::GetTypeID<T>() }));
            AssetBuilder::AssetFile file;
            EXPECT_TRUE(AssetBuilder::LoadAssetFile(path, file));
            auto& artifact = file.m_artifacts.front();
            artifact.m_buildSettings.Emplace<T>(value);
            const auto id = artifact.m_assetId;
            EXPECT_TRUE(AssetBuilder::SaveAssetFile(path, file));
            // Reimport preserves the authored value and discovers nested links through its generated serializer.
            EXPECT_TRUE(AssetBuilder::ImportAsset({ {}, path, Rtti::GetTypeID<T>() }));
            EXPECT_TRUE(AssetBuilder::BuildAsset({ path, {}, m_directory }));
            return id;
        }


        void Pump(EntityWorld& world, MaterializationToken token)
        {
            for (uint32_t iteration = 0; iteration < 5000; ++iteration)
            {
                IO::AssetManager::Tick();
                world.BeginUpdate();
                EXPECT_TRUE(world.EndUpdate());
                if (world.GetMaterializationStatus(token).m_state != MaterializationState::kPending)
                    return;

                Threading::Sleep(1);
            }
            FAIL() << "Materialization did not complete";
        }
    };
} // namespace

TEST_F(CollectionAssetsIntegration, ImportedDefinitionsReleaseWhileNestedRuntimeDependenciesRemain)
{
    const auto leaf = BuildDefinition("leaf.asset", Number{ 73 });
    const auto nested = BuildDefinition("nested.asset", AssetComponent{ IO::Link<Number>(leaf), {} });
    const auto soft = Uuid::Random();
    EntityCollection collection;
    EntityRecord record;
    record.m_uuid = Uuid::Random();
    ASSERT_TRUE(record.AddComponent(
        CollectionAssets{ IO::Link<AssetComponent>(nested), IO::Link<Number, IO::DependencyKind::kSoft>(soft) }));
    collection.m_entities.push_back(std::move(record));
    const auto definitionId = BuildDefinition("collection.asset", collection);
    AssetBuilder::AssetFile file;
    ASSERT_TRUE(AssetBuilder::LoadAssetFile(m_directory / "collection.asset", file));
    const auto& dependencies = file.m_artifacts.front().m_dependencies;
    ASSERT_EQ(dependencies.size(), 2);
    EXPECT_EQ(dependencies[0].m_assetId, nested);
    EXPECT_EQ(dependencies[0].m_expectedTypeId, Rtti::GetTypeID<AssetComponent>());
    EXPECT_EQ(dependencies[0].m_kind, IO::DependencyKind::kHard);
    EXPECT_EQ(dependencies[1].m_assetId, soft);
    EXPECT_EQ(dependencies[1].m_kind, IO::DependencyKind::kSoft);
    IO::ArtifactStore::SetCatalogSource(m_directory);
    IO::AssetManager::Init();
    auto shutdown = festd::defer([] {
        IO::AssetManager::Shutdown();
    });
    EntityWorld world;
    world.Components().Register<CollectionAssets>();
    auto& registry = world.CreateRegistry();
    const auto first = world.SpawnCollection(registry, definitionId);
    const auto second = world.SpawnCollection(registry, definitionId);
    Pump(world, first);
    Pump(world, second);
    ASSERT_EQ(world.GetMaterializationStatus(first).m_state, MaterializationState::kReady);
    ASSERT_EQ(world.GetMaterializationStatus(second).m_state, MaterializationState::kReady);
    IO::AssetManager::Tick();
    auto* definition = IO::AssetManager::FindAssetSlot(definitionId);
    ASSERT_NE(definition, nullptr);
    EXPECT_EQ(definition->m_strongRefCount.load(), 0);
    EXPECT_EQ(definition->m_instance.load(), nullptr);
    auto* dependency = IO::AssetManager::FindAssetSlot(nested);
    auto* leafSlot = IO::AssetManager::FindAssetSlot(leaf);
    ASSERT_NE(dependency, nullptr);
    ASSERT_NE(leafSlot, nullptr);
    EXPECT_EQ(dependency->m_strongRefCount.load(), 2);
    EXPECT_EQ(leafSlot->m_strongRefCount.load(), 2);
    EXPECT_EQ(IO::AssetManager::FindAssetSlot(soft), nullptr);
    auto leafRead = IO::AssetHandle<Number>(leafSlot).Read();
    ASSERT_NE(leafRead.Get(), nullptr);
    EXPECT_EQ(leafRead->m_value, 73);
    world.CancelMaterialization(first);
    IO::AssetManager::Tick();
    EXPECT_EQ(dependency->m_strongRefCount.load(), 1);
    world.RemoveRegistry(registry);
    IO::AssetManager::Tick();
    EXPECT_EQ(dependency->m_strongRefCount.load(), 0);
    EXPECT_EQ(leafSlot->m_strongRefCount.load(), 0);
}


TEST_F(CollectionAssetsIntegration, PersistentAssetDuplicateCancellationAndReload)
{
    EntityCollection collection;
    EntityRecord record;
    record.m_uuid = Uuid::Random();
    ASSERT_TRUE(record.AddComponent(Number{ 91 }));
    collection.m_entities.push_back(std::move(record));
    const auto collectionId = BuildDefinition("collection.asset", collection);
    EntityCollectionInstanceAsset placement;
    placement.m_collection = IO::Link<EntityCollection>(collectionId);
    ASSERT_TRUE(placement.UpdateBindings(collection));
    const auto placementId = BuildDefinition("placement.asset", placement);
    IO::ArtifactStore::SetCatalogSource(m_directory);
    IO::AssetManager::Init();
    auto shutdown = festd::defer([] {
        IO::AssetManager::Shutdown();
    });
    EntityWorld world;
    world.Components().Register<Number>();
    auto& registry = world.CreateRegistry();
    auto canceled = world.LoadPlacement(registry, placementId);
    EXPECT_EQ(canceled, world.LoadPlacement(registry, placementId));
    world.CancelMaterialization(canceled);
    auto first = world.LoadPlacement(registry, placementId);
    EXPECT_EQ(first, world.LoadPlacement(registry, placementId));
    Pump(world, first);
    ASSERT_EQ(world.GetMaterializationStatus(first).m_state, MaterializationState::kReady);
    EXPECT_EQ(world.GetEntityCount(), 2);
    const auto oldId = world.Find(placement.m_bindings.front().m_entityUuid)->GetID();
    world.RemoveRegistry(registry);
    auto& reloadedOwner = world.CreateRegistry();
    auto reloaded = world.LoadPlacement(reloadedOwner, placementId);
    Pump(world, reloaded);
    ASSERT_EQ(world.GetMaterializationStatus(reloaded).m_state, MaterializationState::kReady);
    const auto* entity = world.Find(placement.m_bindings.front().m_entityUuid);
    ASSERT_NE(entity, nullptr);
    EXPECT_NE(entity->GetID(), oldId);
    EXPECT_EQ(entity->FindComponent<const Number>()->m_value, 91);
}

TEST_F(CollectionAssetsIntegration, RuntimeTypeConstraintsKeepValidResidencyIndependent)
{
    const auto id = BuildDefinition("number.asset", Number{ 31 });
    IO::ArtifactStore::SetCatalogSource(m_directory);
    IO::AssetManager::Init();
    auto cleanup = festd::defer([] {
        IO::AssetManager::Shutdown();
    });
    EntityAssetServices services;
    EntityResidencySet residency(services);
    residency.Add(id, Rtti::GetTypeID<AssetComponent>());
    residency.Add(id, Rtti::GetTypeID<Number>());
    residency.Add(id, Rtti::GetTypeID<Number>());
    EXPECT_EQ(residency.GetAcquisitionCount(), 2);
    for (uint32_t iteration = 0; iteration < 5000; ++iteration)
    {
        IO::AssetManager::Tick();
        if (residency.Poll(id, Rtti::GetTypeID<Number>()) != LifecycleResult::kPending)
            break;

        Threading::Sleep(1);
    }
    EXPECT_EQ(residency.Poll(id, Rtti::GetTypeID<Number>()), LifecycleResult::kSucceeded);
    EXPECT_EQ(residency.Poll(id, Rtti::GetTypeID<AssetComponent>()), LifecycleResult::kFailed);
    residency.Remove(id, Rtti::GetTypeID<AssetComponent>());
    EXPECT_EQ(residency.GetAcquisitionCount(), 1);
    residency.Remove(id, Rtti::GetTypeID<Number>());
    EXPECT_EQ(residency.GetAcquisitionCount(), 1);
    residency.Remove(id, Rtti::GetTypeID<Number>());
    EXPECT_EQ(residency.GetAcquisitionCount(), 0);
}


TEST_F(CollectionAssetsIntegration, FailedDefinitionRetainsCoreDiagnosticAfterRequestRelease)
{
    const auto id = BuildDefinition("number.asset", Number{ 31 });
    IO::ArtifactStore::SetCatalogSource(m_directory);
    IO::AssetManager::Init();
    auto cleanup = festd::defer([] {
        IO::AssetManager::Shutdown();
    });
    auto loaded = IO::AssetManager::LoadAsset(IO::Link<Number>(id));
    for (uint32_t iteration = 0; iteration < 5000 && !loaded.IsCompleted(); ++iteration)
    {
        IO::AssetManager::Tick();
        Threading::Sleep(1);
    }
    ASSERT_EQ(loaded.GetResult(), IO::AssetLoadResult::kSucceeded);

    EntityWorld world;
    auto& registry = world.CreateRegistry();
    const auto token = world.SpawnCollection(registry, id);
    Pump(world, token);
    const auto status = world.GetMaterializationStatus(token);
    ASSERT_EQ(status.m_state, MaterializationState::kFailed);
    EXPECT_FALSE(status.m_error.empty());
    EXPECT_NE(status.m_error, "Entity asset unavailable");
    const festd::string error(status.m_error);
    IO::AssetManager::Tick();
    world.BeginUpdate();
    ASSERT_TRUE(world.EndUpdate());
    EXPECT_EQ(world.GetMaterializationStatus(token).m_error, error);
    EXPECT_EQ(world.GetEntityCount(), 0);
}


TEST_F(CollectionAssetsIntegration, WorldAndConcreteSnapshotRoundTripThroughDefaultAssetStreamer)
{
    EntityWorldAsset definition;
    auto& group = definition.m_registries.emplace_back();
    group.m_key = Uuid::Random();
    EntityRecord record;
    record.m_uuid = Uuid::Random();
    const Uuid uuid = record.m_uuid;
    ASSERT_TRUE(record.AddComponent(Number{ 31 }));
    group.m_entities.m_entities.push_back(std::move(record));
    const auto worldId = BuildDefinition("world.asset", definition);
    IO::ArtifactStore::SetCatalogSource(m_directory);
    IO::AssetManager::Init();
    auto cleanup = festd::defer([] {
        IO::AssetManager::Shutdown();
    });
    auto load = [](IO::AssetRequest& request) {
        for (uint32_t i = 0; i < 5000 && !request.IsCompleted(); ++i)
        {
            IO::AssetManager::Tick();
            Threading::Sleep(1);
        }
        EXPECT_EQ(request.GetResult(), IO::AssetLoadResult::kSucceeded);
    };
    auto worldRequest = IO::AssetManager::LoadAsset(IO::Link<EntityWorldAsset>(worldId));
    load(worldRequest);
    EntityWorld world;
    world.Components().Register<Number>();
    {
        const auto value = IO::AssetHandle<EntityWorldAsset>(worldRequest.GetAssetSlot()).Read();
        ASSERT_TRUE(value);
        ASSERT_TRUE(world.LoadDefinition(*value.Get()));
    }
    ASSERT_TRUE(world.CommitBootstrap());
    worldRequest.Reset();
    world.Find(uuid)->FindComponent<Number>()->m_value = 55;
    EntityWorldSnapshotAsset snapshot;
    ASSERT_TRUE(world.CaptureSnapshot(snapshot));
    const auto snapshotId = BuildDefinition("snapshot.asset", snapshot);
    world.Clear();
    auto snapshotRequest = IO::AssetManager::LoadAsset(IO::Link<EntityWorldSnapshotAsset>(snapshotId));
    load(snapshotRequest);
    EntityWorld restored;
    restored.Components().Register<Number>();
    {
        const auto value = IO::AssetHandle<EntityWorldSnapshotAsset>(snapshotRequest.GetAssetSlot()).Read();
        ASSERT_TRUE(value);
        ASSERT_TRUE(restored.RestoreSnapshot(*value.Get()));
    }
    ASSERT_TRUE(restored.CommitBootstrap());
    snapshotRequest.Reset();
    ASSERT_NE(restored.Find(uuid), nullptr);
    EXPECT_EQ(restored.Find(uuid)->FindComponent<const Number>()->m_value, 55);
    EXPECT_EQ(restored.FindRegistry(group.m_key)->GetKey(), group.m_key);
}
