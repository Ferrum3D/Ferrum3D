#include <Core/IO/MemoryStream.h>
#include <Core/Serialization/BinarySerialization.h>
#include <Core/Serialization/JsonSerialization.h>
#include <EntityTestTypes.h>
#include <gtest/gtest.h>

using namespace FE;
using namespace FE::Framework;
using namespace FE::Framework::Tests;

namespace
{
    const Uuid kSource("B9DBE80D-6AB0-486A-AB00-000000001001");
    const Uuid kCollectionChild("B9DBE80D-6AB0-486A-AB00-000000001002");
    const Uuid kExternal("B9DBE80D-6AB0-486A-AB00-000000001003");
    const IO::AssetID kPlacement("B9DBE80D-6AB0-486A-AB00-000000001004");
    const IO::AssetID kCollection("B9DBE80D-6AB0-486A-AB00-000000001005");

    struct CollectionFixture : testing::Test
    {
        EntityWorld m_world;
        EntityRegistry& m_registry = m_world.CreateRegistry();
        EntityCollection m_collection;
        void SetUp() override
        {
            m_world.Components().Register<Number>();
            m_world.Components().Register<ReferenceComponent>();
            EntityRecord parent;
            parent.m_uuid = kSource;
            parent.m_name = "Parent";
            ASSERT_TRUE(parent.AddComponent(Number{ 42 }));
            m_collection.m_entities.push_back(std::move(parent));
            EntityRecord child;
            child.m_uuid = kCollectionChild;
            child.m_parentUuid = kSource;
            ASSERT_TRUE(child.AddComponent(ReferenceComponent{ { kSource }, { kExternal, kPlacement } }));
            m_collection.m_entities.push_back(std::move(child));
        }


        void Tick()
        {
            m_world.BeginUpdate();
            EXPECT_TRUE(m_world.EndUpdate());
        }


        EntityCollectionInstanceAsset Placement()
        {
            EntityCollectionInstanceAsset placement;
            placement.m_collection = IO::Link<EntityCollection>(kCollection);
            EXPECT_TRUE(placement.UpdateBindings(m_collection));
            return placement;
        }
    };
} // namespace

TEST_F(CollectionFixture, RepeatedSpawnOwnsDataAndRemapsOnlyInternalReferences)
{
    const auto first = m_world.SpawnCollection(m_registry, m_collection);
    const auto second = m_world.SpawnCollection(m_registry, m_collection);
    m_collection = {};
    EXPECT_EQ(m_world.GetEntityCount(), 0);
    Tick();
    EXPECT_EQ(m_world.GetMaterializationStatus(first).m_state, MaterializationState::kReady);
    EXPECT_EQ(m_world.GetMaterializationStatus(second).m_state, MaterializationState::kReady);

    const auto a = m_world.GetMaterializationBindings(first);
    const auto b = m_world.GetMaterializationBindings(second);
    ASSERT_EQ(a.size(), 2);
    ASSERT_EQ(b.size(), 2);
    EXPECT_NE(a[0].m_entityUuid, b[0].m_entityUuid);

    auto* parent = m_world.Find(a[0].m_entityUuid);
    auto* child = m_world.Find(a[1].m_entityUuid);
    ASSERT_NE(parent, nullptr);
    ASSERT_NE(child, nullptr);
    EXPECT_EQ(child->GetParent(), parent);
    EXPECT_EQ(parent->FindComponent<Number>()->m_value, 42);

    const auto* references = child->FindComponent<ReferenceComponent>();
    EXPECT_EQ(references->m_internal.Resolve(m_world), parent);
    EXPECT_EQ(references->m_external.m_uuid, kExternal);
    EXPECT_EQ(references->m_external.m_placementAsset, kPlacement);
    EXPECT_EQ(references->m_external.Resolve(m_world), nullptr);
    EXPECT_EQ(m_world.GetEntityCount(), 6);
}


TEST_F(CollectionFixture, PersistentMembershipDeduplicatesAndReloadsStableBindings)
{
    auto placement = Placement();
    const auto first = m_world.LoadPlacement(m_registry, kPlacement, placement, m_collection);
    EXPECT_EQ(first, m_world.LoadPlacement(m_registry, kPlacement, placement, m_collection));
    Tick();
    ASSERT_EQ(m_world.GetMaterializationStatus(first).m_state, MaterializationState::kReady);
    EXPECT_EQ(first, m_world.LoadPlacement(m_registry, kPlacement, placement, m_collection));

    const auto oldId = m_world.Find(placement.m_bindings[0].m_entityUuid)->GetID();
    EntityReference reference{ placement.m_bindings[0].m_entityUuid, kPlacement };
    m_world.RemoveRegistry(m_registry);
    EXPECT_EQ(reference.Resolve(m_world), nullptr);

    auto& owner = m_world.CreateRegistry();
    const auto reloaded = m_world.LoadPlacement(owner, kPlacement, placement, m_collection);
    Tick();
    ASSERT_EQ(m_world.GetMaterializationStatus(reloaded).m_state, MaterializationState::kReady);
    ASSERT_NE(reference.Resolve(m_world), nullptr);
    EXPECT_NE(reference.Resolve(m_world)->GetID(), oldId);
    EXPECT_EQ(m_world.Find(oldId), nullptr);
}


TEST_F(CollectionFixture, InvalidComponentsAndBindingsRejectWithoutAllocatingRows)
{
    auto invalid = m_collection;
    invalid.m_entities[1].m_components[0].Reset();

    auto empty = m_world.SpawnCollection(m_registry, invalid);
    invalid = m_collection;
    invalid.m_entities[0].m_components[0].Emplace<AssetComponent>();

    auto unknown = m_world.SpawnCollection(m_registry, invalid);
    invalid = m_collection;
    invalid.m_entities[1].m_components.push_back(invalid.m_entities[1].m_components[0]);

    auto duplicate = m_world.SpawnCollection(m_registry, invalid);
    auto placement = Placement();
    placement.m_bindings[0].m_entityUuid = placement.m_rootUuid;

    auto bindings = m_world.LoadPlacement(m_registry, kPlacement, placement, m_collection);
    Tick();
    for (auto token : { empty, unknown, duplicate, bindings })
    {
        EXPECT_EQ(m_world.GetMaterializationStatus(token).m_state, MaterializationState::kFailed);
        EXPECT_EQ(m_world.GetMaterializationStatus(token).m_error, "Invalid entity asset");
    }
    EXPECT_EQ(m_world.GetEntityCount(), 0);
    EXPECT_EQ(m_world.GetChunkCount(), 0);
}


TEST_F(CollectionFixture, CancelBeforePublicationAndUnloadInvalidatePendingOperations)
{
    auto canceled = m_world.SpawnCollection(m_registry, m_collection);
    auto unloaded = m_world.SpawnCollection(m_registry, m_collection);
    m_world.CancelMaterialization(canceled);
    m_world.RemoveRegistry(m_registry);
    Tick();
    EXPECT_EQ(m_world.GetMaterializationStatus(canceled).m_state, MaterializationState::kCanceled);
    EXPECT_EQ(m_world.GetMaterializationStatus(unloaded).m_state, MaterializationState::kCanceled);
    EXPECT_EQ(m_world.GetEntityCount(), 0);
    EntityWorld other;
    EXPECT_EQ(other.GetMaterializationStatus(unloaded).m_state, MaterializationState::kFailed);
}


TEST_F(CollectionFixture, BindingMaintenanceAndCopiesKeepSourceIdentity)
{
    auto placement = Placement();
    const auto oldUuid = placement.m_bindings[0].m_entityUuid;
    m_collection.m_entities[1].m_parentUuid = Uuid::kNull;
    m_collection.m_entities.erase(m_collection.m_entities.begin());
    EntityRecord added;
    added.m_uuid = Uuid::Random();
    m_collection.m_entities.push_back(added);

    const auto keptUuid = placement.m_bindings[1].m_entityUuid;
    ASSERT_TRUE(placement.UpdateBindings(m_collection));
    EXPECT_EQ(placement.m_bindings[0].m_entityUuid, keptUuid);
    EXPECT_NE(placement.m_bindings[1].m_entityUuid, oldUuid);

    auto copy = placement;
    ASSERT_TRUE(copy.MakeIndependentCopy());
    EXPECT_NE(copy.m_rootUuid, placement.m_rootUuid);
    EXPECT_EQ(copy.m_bindings[0].m_sourceUuid, placement.m_bindings[0].m_sourceUuid);
    EXPECT_NE(copy.m_bindings[0].m_entityUuid, placement.m_bindings[0].m_entityUuid);
    EXPECT_TRUE(copy.Validate(m_collection));
}


TEST_F(CollectionFixture, CollectionSerializationDiscoversNestedAssetLinks)
{
    const auto asset = Uuid::Random();
    EntityRecord record;
    record.m_uuid = Uuid::Random();
    ASSERT_TRUE(
        record.AddComponent(AssetComponent{ IO::Link<Number>(asset), IO::Link<Number, IO::DependencyKind::kSoft>(kExternal) }));
    m_collection.m_entities.push_back(std::move(record));

    uint32_t hard = 0;
    IO::WriteOnlyMemoryStream stream;
    Serialization::TaggedBinaryFormat format;
    Serialization::SerializationContext context(&stream, format, &hard, [](void* data, Uuid, Rtti::TypeID, uint32_t kind) {
        if (kind == festd::to_underlying(IO::DependencyKind::kHard))
            ++*static_cast<uint32_t*>(data);
    });
    ASSERT_EQ(context.Store(m_collection), Serialization::ResultCode::kSuccess);
    EXPECT_EQ(hard, 1);
    festd::pmr::vector<std::byte> bytes;
    stream.DumpAll(bytes);
    IO::ReadOnlyMemoryStream input(bytes);
    Serialization::TaggedBinaryFormat loadFormat;
    Serialization::DeserializationContext load(&input, loadFormat);
    EntityCollection restored;
    ASSERT_EQ(load.Load(restored), Serialization::ResultCode::kSuccess);
    EXPECT_TRUE(restored.Validate());
    ASSERT_EQ(restored.m_entities.size(), m_collection.m_entities.size());
    EXPECT_EQ(restored.m_entities.front().m_components.front().TryGet<Number>()->m_value, 42);
    const auto* restoredAssets = restored.m_entities.back().m_components.front().TryGet<AssetComponent>();
    ASSERT_NE(restoredAssets, nullptr);
    EXPECT_EQ(restoredAssets->m_hard.GetAssetID(), asset);
    EXPECT_EQ(restoredAssets->m_soft.GetAssetID(), kExternal);
}


TEST_F(CollectionFixture, PlacementCopyRemapsRootReferences)
{
    auto placement = Placement();
    const auto boundUuid = placement.m_bindings.front().m_entityUuid;
    ASSERT_TRUE(placement.m_root.m_entities.front().AddComponent(
        ReferenceComponent{ { boundUuid, kPlacement }, { kExternal, kPlacement } }));

    auto copy = placement;
    ASSERT_TRUE(copy.MakeIndependentCopy());

    auto token = m_world.LoadPlacement(m_registry, Uuid::Random(), copy, m_collection);
    Tick();
    ASSERT_EQ(m_world.GetMaterializationStatus(token).m_state, MaterializationState::kReady);

    const auto* root = m_world.Find(copy.m_rootUuid);
    ASSERT_NE(root, nullptr);

    const auto* references = root->FindComponent<const ReferenceComponent>();
    ASSERT_NE(references, nullptr);
    EXPECT_EQ(references->m_internal.m_uuid, copy.m_bindings.front().m_entityUuid);
    EXPECT_FALSE(references->m_internal.m_placementAsset.IsValid());
    EXPECT_EQ(references->m_external.m_uuid, kExternal);
    EXPECT_EQ(references->m_external.m_placementAsset, kPlacement);
}


TEST_F(CollectionFixture, FailedPlacementCanRetryWithTheSameAssetIdentity)
{
    auto placement = Placement();
    auto invalid = placement;
    invalid.m_bindings.front().m_sourceUuid = Uuid::Random();
    const auto failed = m_world.LoadPlacement(m_registry, kPlacement, invalid, m_collection);
    Tick();
    ASSERT_EQ(m_world.GetMaterializationStatus(failed).m_state, MaterializationState::kFailed);
    EXPECT_EQ(m_world.GetEntityCount(), 0);

    const auto retry = m_world.LoadPlacement(m_registry, kPlacement, placement, m_collection);
    EXPECT_NE(retry, failed);
    Tick();
    ASSERT_EQ(m_world.GetMaterializationStatus(retry).m_state, MaterializationState::kReady);
    EXPECT_EQ(m_world.LoadPlacement(m_registry, kPlacement, placement, m_collection), retry);
}


TEST_F(CollectionFixture, ConflictingPlacementRootsAndInvalidHierarchyRejectWithoutDuplicates)
{
    auto placement = Placement();
    auto first = m_world.LoadPlacement(m_registry, kPlacement, placement, m_collection);
    auto conflicting = m_world.LoadPlacement(m_registry, Uuid::Random(), placement, m_collection);
    auto invalid = m_collection;
    invalid.m_entities.front().m_parentUuid = kCollectionChild;

    auto cycle = m_world.SpawnCollection(m_registry, invalid);
    invalid = m_collection;
    invalid.m_entities.front().m_parentUuid = kExternal;

    auto externalParent = m_world.SpawnCollection(m_registry, invalid);
    invalid = m_collection;
    invalid.m_entities.front().m_components.front().Reset();

    auto empty = m_world.SpawnCollection(m_registry, invalid);
    Tick();
    ASSERT_EQ(m_world.GetMaterializationStatus(first).m_state, MaterializationState::kReady);
    for (auto token : { conflicting, cycle, externalParent, empty })
        EXPECT_EQ(m_world.GetMaterializationStatus(token).m_state, MaterializationState::kFailed);
    EXPECT_EQ(m_world.GetEntityCount(), 3);
}


TEST(CollectionMaterialization, PendingDependenciesPreventPublicationAndCancelWithoutLateActivation)
{
    struct Assets final : EntityAssetServices
    {
        uint32_t m_acquisitions = 0;
        bool m_ready = false;
        EntityAssetRequest Acquire(IO::AssetID, Rtti::TypeID) override
        {
            ++m_acquisitions;
            return { {}, 1 };
        }


        LifecycleResult Poll(const EntityAssetRequest&, Rtti::TypeID) override
        {
            return m_ready ? LifecycleResult::kSucceeded : LifecycleResult::kPending;
        }


        void Release(EntityAssetRequest& request) override
        {
            if (request.m_serviceToken)
                --m_acquisitions;
            request.m_serviceToken = 0;
        }
    } assets;
    EntityWorld world(&assets);
    world.Components().Register<AssetComponent>();

    auto& registry = world.CreateRegistry();
    EntityCollection collection;
    EntityRecord record;
    record.m_uuid = Uuid::Random();
    ASSERT_TRUE(record.AddComponent(AssetComponent{ IO::Link<Number>(Uuid::Random()), {} }));
    collection.m_entities.push_back(std::move(record));

    const auto token = world.SpawnCollection(registry, collection);
    ASSERT_TRUE(world.CommitBootstrap());
    EXPECT_EQ(world.GetMaterializationStatus(token).m_state, MaterializationState::kPending);
    ASSERT_EQ(assets.m_acquisitions, 1);

    const auto uuid = world.GetMaterializationBindings(token).front().m_entityUuid;
    EntityReference reference{ uuid };
    EXPECT_EQ(reference.Resolve(world), nullptr);
    EXPECT_NE(reference.Resolve(world, false), nullptr);
    world.CancelMaterialization(token);
    EXPECT_EQ(assets.m_acquisitions, 0);
    assets.m_ready = true;
    ASSERT_TRUE(world.CommitBootstrap());
    EXPECT_EQ(reference.Resolve(world, false), nullptr);
    EXPECT_EQ(world.GetEntityCount(), 0);
}


TEST_F(CollectionFixture, LifecycleCanQueueMoreSpawnsWithoutInvalidatingCurrentOperation)
{
    m_world.Components().Register<SpawnFromLoad>();
    EntityCollection triggered;
    EntityRecord record;
    record.m_uuid = Uuid::Random();
    ASSERT_TRUE(record.AddComponent(SpawnFromLoad{}));
    triggered.m_entities.push_back(std::move(record));
    SpawnFromLoad::s_collection = &m_collection;
    SpawnFromLoad::s_operations.clear();

    auto cleanup = festd::defer([] {
        SpawnFromLoad::s_collection = nullptr;
        SpawnFromLoad::s_operations.clear();
    });

    auto first = m_world.SpawnCollection(m_registry, triggered);
    Tick();
    EXPECT_EQ(m_world.GetMaterializationStatus(first).m_state, MaterializationState::kReady);
    ASSERT_EQ(SpawnFromLoad::s_operations.size(), 16);
    for (const auto token : SpawnFromLoad::s_operations)
        EXPECT_EQ(m_world.GetMaterializationStatus(token).m_state, MaterializationState::kPending);
    for (uint32_t frame = 0; frame < 8; ++frame)
    {
        Tick();
        bool ready = true;
        for (const auto token : SpawnFromLoad::s_operations)
            ready &= m_world.GetMaterializationStatus(token).m_state == MaterializationState::kReady;

        if (ready)
            break;
    }

    for (const auto token : SpawnFromLoad::s_operations)
        EXPECT_EQ(m_world.GetMaterializationStatus(token).m_state, MaterializationState::kReady);
    EXPECT_EQ(m_world.GetEntityCount(), 50);
}


TEST_F(CollectionFixture, DestroyingPlacementRootRetiresMembershipAndPermitsExplicitReload)
{
    auto placement = Placement();
    auto first = m_world.LoadPlacement(m_registry, kPlacement, placement, m_collection);
    Tick();
    ASSERT_EQ(m_world.GetMaterializationStatus(first).m_state, MaterializationState::kReady);
    EntityCommandList commands(m_world);
    commands.Destroy(m_world.GetMaterializationStatus(first).m_root);
    m_world.Submit(std::move(commands));
    Tick();
    EXPECT_EQ(m_world.GetMaterializationStatus(first).m_state, MaterializationState::kCanceled);

    auto reloaded = m_world.LoadPlacement(m_registry, kPlacement, placement, m_collection);
    EXPECT_NE(reloaded, first);
    Tick();
    EXPECT_EQ(m_world.GetMaterializationStatus(reloaded).m_state, MaterializationState::kReady);
}


TEST_F(CollectionFixture, ReferenceSerializerUsesReflectedFieldNames)
{
    const EntityReference source{ kSource, kPlacement };
    IO::WriteOnlyMemoryStream stream;
    Serialization::JsonFormat format;
    Serialization::SerializationContext writer(&stream, format);
    ASSERT_EQ(writer.Store(source), Serialization::ResultCode::kSuccess);
    festd::pmr::vector<std::byte> bytes;
    stream.DumpAll(bytes);

    const festd::string_view text(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    EXPECT_NE(text.find("\"m_uuid\""), text.end());
    EXPECT_NE(text.find("\"m_placementAsset\""), text.end());
    EXPECT_EQ(text.find("\"uuid\""), text.end());
    IO::ReadOnlyMemoryStream input(bytes);
    Serialization::JsonFormat loadFormat;
    Serialization::DeserializationContext reader(&input, loadFormat);
    EntityReference restored;
    ASSERT_EQ(reader.Load(restored), Serialization::ResultCode::kSuccess);
    EXPECT_EQ(restored.m_uuid, source.m_uuid);
    EXPECT_EQ(restored.m_placementAsset, source.m_placementAsset);
}


TEST_F(CollectionFixture, FailedReferenceDeserializationDoesNotRemap)
{
    const EntityReference source{ kSource, kPlacement };
    IO::WriteOnlyMemoryStream stream;
    Serialization::PackedBinaryFormat format;
    Serialization::SerializationContext writer(&stream, format);
    ASSERT_EQ(writer.Store(source), Serialization::ResultCode::kSuccess);
    festd::pmr::vector<std::byte> bytes;
    stream.DumpAll(bytes);
    bytes.pop_back();
    IO::ReadOnlyMemoryStream input(bytes);
    Serialization::PackedBinaryFormat loadFormat;
    Serialization::DeserializationContext reader(&input, loadFormat);

    uint32_t remaps = 0;
    reader.SetObjectReferenceRemapper(&remaps, [](void* data, Uuid) {
        ++*static_cast<uint32_t*>(data);
        return kExternal;
    });
    EntityReference restored;
    EXPECT_NE(reader.Load(restored), Serialization::ResultCode::kSuccess);
    EXPECT_EQ(remaps, 0);
}


TEST_F(CollectionFixture, RuntimeMaterializationIsBoundedAcrossRepeatedCommitsAndCanBeCanceled)
{
    EntityCollection collection;
    for (uint32_t i = 0; i < 100; ++i)
    {
        EntityRecord record;
        record.m_uuid = Uuid::Random();
        ASSERT_TRUE(record.AddComponent(Number{ static_cast<int32_t>(i) }));
        collection.m_entities.push_back(std::move(record));
    }

    const auto token = m_world.LoadEntities(m_registry, collection);
    m_world.BeginFrame();
    m_world.BeginUpdate();
    EXPECT_EQ(m_world.GetEntityCount(), 32);
    ASSERT_TRUE(m_world.Commit());
    EXPECT_EQ(m_world.GetEntityCount(), 32);
    EXPECT_EQ(m_world.GetMaterializationStatus(token).m_state, MaterializationState::kPending);
    ASSERT_TRUE(m_world.EndUpdate());
    Tick();
    EXPECT_EQ(m_world.GetEntityCount(), 32);
    m_world.BeginFrame();
    Tick();
    EXPECT_EQ(m_world.GetEntityCount(), 64);
    m_world.CancelMaterialization(token);
    EXPECT_EQ(m_world.GetEntityCount(), 0);
    EXPECT_EQ(m_world.GetMaterializationStatus(token).m_state, MaterializationState::kCanceled);
    Tick();
    EXPECT_EQ(m_world.GetEntityCount(), 0);
}


TEST_F(CollectionFixture, LargeSubtreeEventuallyPublishesAtomically)
{
    EntityCollection collection;
    const Uuid rootId = Uuid::Random();
    for (uint32_t i = 0; i < 80; ++i)
    {
        EntityRecord record;
        record.m_uuid = i == 0 ? rootId : Uuid::Random();
        record.m_parentUuid = i == 0 ? Uuid::kNull : rootId;
        ASSERT_TRUE(record.AddComponent(Number{ static_cast<int32_t>(i) }));
        collection.m_entities.push_back(std::move(record));
    }

    const auto token = m_world.SpawnCollection(m_registry, collection);
    uint32_t previousCount = 0;
    for (uint32_t frame = 0; frame < 24; ++frame)
    {
        Tick();
        EXPECT_LE(m_world.GetEntityCount() - previousCount, 32);
        previousCount = m_world.GetEntityCount();
        const bool ready = m_world.GetMaterializationStatus(token).m_state == MaterializationState::kReady;
        uint32_t active = 0;
        for (const auto& binding : m_world.GetMaterializationBindings(token))
            active += m_world.Find(binding.m_entityUuid, true) != nullptr;

        EXPECT_EQ(active, ready ? 80u : 0u);
        if (ready)
            return;
    }

    FAIL() << "Budgeted subtree never finished";
}


TEST_F(CollectionFixture, DestroyingPartiallyConstructedMembershipFailsCleanly)
{
    EntityCollection collection;
    for (uint32_t i = 0; i < 64; ++i)
    {
        EntityRecord record;
        record.m_uuid = Uuid::Random();
        ASSERT_TRUE(record.AddComponent(Number{ static_cast<int32_t>(i) }));
        collection.m_entities.push_back(std::move(record));
    }

    const auto token = m_world.LoadEntities(m_registry, collection);
    Tick();
    ASSERT_EQ(m_world.GetEntityCount(), 32);
    const auto uuid = m_world.GetMaterializationBindings(token).front().m_entityUuid;
    EntityCommandList commands(m_world);
    commands.Destroy(m_world.Find(uuid, false)->GetID());
    m_world.Submit(std::move(commands));
    Tick();
    EXPECT_EQ(m_world.GetMaterializationStatus(token).m_state, MaterializationState::kFailed);
    EXPECT_EQ(m_world.GetEntityCount(), 0);
}
