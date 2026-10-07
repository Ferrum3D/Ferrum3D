#include <AssetBuilder/AssetFile.h>
#include <AssetBuilder/EntityImport.h>
#include <Core/IO/Artifact.h>
#include <Core/Strings/Format.h>
#include <Core/Threading/Thread.h>
#include <EntityTestTypes.h>
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
            m_directory = IO::Path(FE_ENTITY_ASSET_TEST_OUTPUT) / Fmt::Format("{}", NewEntityUuid());
        }
        template<class T>
        IO::AssetID BuildDefinition(const char* name, const T& value)
        {
            const IO::Path path = m_directory / name;
            AssetBuilder::AssetFile file;
            auto& artifact = file.m_artifacts.emplace_back();
            artifact.m_productKey = "Serialized/Primary";
            artifact.m_name = name;
            artifact.m_assetId = NewEntityUuid();
            artifact.m_assetTypeId = Rtti::GetTypeID<T>();
            artifact.m_buildSettings.Emplace<T>(value);
            const auto id = artifact.m_assetId;
            EXPECT_TRUE(AssetBuilder::SaveAssetFile(path, file));
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
    const auto soft = NewEntityUuid();
    EntityCollection collection;
    EntityRecord record;
    record.m_uuid = NewEntityUuid();
    ASSERT_TRUE(collection.CookComponent(
        record,
        CollectionAssets{ IO::Link<AssetComponent>(nested), IO::Link<Number, IO::DependencyKind::kSoft>(soft) }));
    collection.m_entities.push_back(std::move(record));
    auto unsupported = collection;
    ++unsupported.m_entities.front().m_components.front().m_version;
    EXPECT_FALSE(AssetBuilder::ImportEntityCollection(m_directory / "unsupported.asset", unsupported));
    auto missingMetadata = collection;
    missingMetadata.m_entities.front().m_components.front().m_dependencies.clear();
    EXPECT_FALSE(AssetBuilder::ImportEntityCollection(m_directory / "missing-dependencies.asset", missingMetadata));
    const auto path = m_directory / "collection.asset";
    ASSERT_TRUE(AssetBuilder::ImportEntityCollection(path, collection));
    AssetBuilder::AssetFile file;
    ASSERT_TRUE(AssetBuilder::LoadAssetFile(path, file));
    const auto definitionId = file.m_artifacts.front().m_assetId;
    ASSERT_TRUE(AssetBuilder::BuildAsset({ path, {}, m_directory }));
    IO::ArtifactStore::SetCatalogSource(m_directory);
    IO::AssetManager::Init();
    RegisterEntityAssetStreamers();
    auto shutdown = festd::defer([] {
        UnregisterEntityAssetStreamers();
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
    record.m_uuid = NewEntityUuid();
    ASSERT_TRUE(collection.CookComponent(record, Number{ 91 }));
    collection.m_entities.push_back(std::move(record));
    const auto collectionPath = m_directory / "collection.asset";
    ASSERT_TRUE(AssetBuilder::ImportEntityCollection(collectionPath, collection));
    AssetBuilder::AssetFile file;
    ASSERT_TRUE(AssetBuilder::LoadAssetFile(collectionPath, file));
    const auto collectionId = file.m_artifacts.front().m_assetId;
    ASSERT_TRUE(AssetBuilder::BuildAsset({ collectionPath, {}, m_directory }));
    EntityCollectionInstanceAsset placement;
    placement.m_collection = IO::Link<EntityCollection>(collectionId);
    ASSERT_TRUE(placement.UpdateBindings(collection));
    const auto path = m_directory / "placement.asset";
    ASSERT_TRUE(AssetBuilder::ImportEntityPlacement(path, placement, collection));
    ASSERT_TRUE(AssetBuilder::LoadAssetFile(path, file));
    const auto placementId = file.m_artifacts.front().m_assetId;
    ASSERT_TRUE(AssetBuilder::BuildAsset({ path, {}, m_directory }));
    IO::ArtifactStore::SetCatalogSource(m_directory);
    IO::AssetManager::Init();
    RegisterEntityAssetStreamers();
    auto shutdown = festd::defer([] {
        UnregisterEntityAssetStreamers();
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
