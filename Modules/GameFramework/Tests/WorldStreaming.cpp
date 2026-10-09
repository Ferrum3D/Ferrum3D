#include <AssetBuilder/AssetFile.h>
#include <AssetBuilder/AssetPipeline.h>
#include <AssetBuilder/EntityImport.h>
#include <Core/IO/Artifact.h>
#include <Core/Strings/Format.h>
#include <Core/Threading/Thread.h>
#include <Framework/Entities/EntityReference.h>
#include <Framework/Entities/EntityWorldAsset.h>
#include <GameFramework/TransformComponents.h>
#include <GameFramework/TransformationSystem.h>
#include <GameFramework/WorldStreamingService.h>
#include <gtest/gtest.h>

using namespace FE;
using namespace FE::Framework;
using namespace FE::GameFramework;

TEST(WorldStreaming, DiskPlacementReloadPreservesUuidAndCancelPreventsLateEntities)
{
    const IO::Path directory = IO::Path(FE_GAME_ASSET_TEST_OUTPUT) / Fmt::Format("{}", Uuid::Random());
    EntityCollection collection;
    EntityRecord record;
    record.m_uuid = Uuid::Random();
    ASSERT_TRUE(collection.CookComponent(record, TransformComponent{}));
    collection.m_entities.push_back(record);
    const IO::Path collectionPath = directory / "collection.asset";
    ASSERT_TRUE(AssetBuilder::ImportEntityCollection(collectionPath, collection));
    AssetBuilder::AssetFile file;
    ASSERT_TRUE(AssetBuilder::LoadAssetFile(collectionPath, file));
    const auto collectionId = file.m_artifacts[0].m_assetId;
    ASSERT_TRUE(AssetBuilder::BuildAsset({ collectionPath, {}, directory }));
    EntityCollectionInstanceAsset placement;
    placement.m_collection = IO::Link<EntityCollection>(collectionId);
    ASSERT_TRUE(placement.UpdateBindings(collection));
    const IO::Path placementPath = directory / "placement.asset";
    ASSERT_TRUE(AssetBuilder::ImportEntityPlacement(placementPath, placement, collection));
    ASSERT_TRUE(AssetBuilder::LoadAssetFile(placementPath, file));
    const auto placementId = file.m_artifacts[0].m_assetId;
    ASSERT_TRUE(AssetBuilder::BuildAsset({ placementPath, {}, directory }));
    IO::ArtifactStore::SetCatalogSource(directory);
    IO::AssetManager::Init();
    auto cleanup = festd::defer([] {
        IO::AssetManager::Shutdown();
    });
    TransformationSystem transforms;
    WorldStreamingService streaming;
    EntityWorld world;
    world.AddSystem(transforms);
    world.AddService(streaming);
    auto removeSystems = festd::defer([&] {
        world.Clear();
        world.RemoveService(streaming);
        world.RemoveSystem(transforms);
    });
    auto tick = [&] {
        IO::AssetManager::Tick();
        world.BeginUpdate();
        world.SchedulePhase(GameFramework::Phases::Transformation);
        EXPECT_TRUE(world.ExecuteSchedule());
        EXPECT_TRUE(world.EndUpdate());
    };
    auto pump = [&] {
        for (uint32_t i = 0; i < 5000; ++i)
        {
            tick();
            if (streaming.GetStatus(placementId).m_state != MaterializationState::kPending)
                return;

            Threading::Sleep(1);
        }
        ADD_FAILURE() << "Placement did not finish loading";
    };
    const Uuid key = Uuid::Random();
    streaming.Load(key, placementId);
    pump();
    ASSERT_EQ(streaming.GetStatus(placementId).m_state, MaterializationState::kReady);
    const auto uuid = placement.m_bindings[0].m_entityUuid;
    const EntityID old = world.Find(uuid)->GetID();
    EntityReference reference;
    reference.m_uuid = uuid;
    ASSERT_EQ(reference.Resolve(world), world.Find(uuid));
    streaming.Unload(key);
    tick();
    EXPECT_EQ(streaming.GetStatus(placementId).m_state, MaterializationState::kCanceled);
    EXPECT_EQ(reference.Resolve(world), nullptr);
    EXPECT_EQ(world.GetEntityCount(), 0);
    streaming.Load(key, placementId);
    pump();
    ASSERT_EQ(streaming.GetStatus(placementId).m_state, MaterializationState::kReady);
    ASSERT_NE(reference.Resolve(world), nullptr);
    EXPECT_NE(reference.Resolve(world)->GetID(), old);
    EXPECT_EQ(world.Find(old), nullptr);
    streaming.Unload(key);
    tick();
    streaming.Load(key, placementId);
    streaming.Unload(key);
    tick();
    EXPECT_EQ(streaming.GetStatus(placementId).m_state, MaterializationState::kCanceled);
    for (uint32_t i = 0; i < 20; ++i)
        tick();

    EXPECT_EQ(world.GetEntityCount(), 0);
    EXPECT_EQ(world.FindRegistry(key), nullptr);
    const auto missing = Uuid::Random();
    streaming.Load(key, missing);
    for (uint32_t i = 0; i < 5000; ++i)
    {
        tick();
        if (streaming.GetStatus(missing).m_state == MaterializationState::kFailed)
            break;

        Threading::Sleep(1);
    }

    EXPECT_EQ(streaming.GetStatus(missing).m_state, MaterializationState::kFailed);
    EXPECT_FALSE(streaming.GetStatus(missing).m_error.empty());
    EXPECT_EQ(world.GetEntityCount(), 0);
}
