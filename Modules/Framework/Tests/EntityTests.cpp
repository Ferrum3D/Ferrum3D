#include <Core/CLI/CommandLine.h>
#include <Core/IO/Artifact.h>
#include <Core/Jobs/Jobs.h>
#include <Core/Threading/Thread.h>
#include <EntityTestTypes.h>
#include <Framework/Entities/Query.h>
#include <chrono>
#include <gtest/gtest.h>

using namespace FE;
using namespace FE::Framework;
using namespace FE::Framework::Tests;

namespace
{
    void ExpectIncompleteUpdate(EntityWorld& world)
    {
#if FE_DEBUG
        EXPECT_DEATH(world.EndUpdate(), "");
#else
        if (Cli::Check("--ecs-validation"))
            EXPECT_DEATH(world.EndUpdate(), "");
        else
            EXPECT_FALSE(world.EndUpdate());
#endif
    }


    struct ThreadOverlap final
    {
        std::atomic<uint32_t> m_entered = 0;
        uint64_t m_threads[2]{};

        void Meet(const uint32_t participant)
        {
            m_threads[participant] = Threading::GetCurrentThreadID();
            m_entered.fetch_add(1, std::memory_order_release);

            // No fiber yield: overlap must come from another OS thread.
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
            while (m_entered.load(std::memory_order_acquire) < 2 && std::chrono::steady_clock::now() < deadline)
                _mm_pause();

            EXPECT_EQ(m_entered.load(std::memory_order_acquire), 2);
        }


        void Check() const
        {
            EXPECT_NE(m_threads[0], 0);
            EXPECT_NE(m_threads[1], 0);
            EXPECT_NE(m_threads[0], m_threads[1]);
        }
    };


    struct FakeAssets final : EntityAssetServices
    {
        uint32_t m_acquires = 0;
        uint32_t m_releases = 0;
        LifecycleResult m_status = LifecycleResult::kPending;
        EntityAssetRequest Acquire(IO::AssetID, Rtti::TypeID) override
        {
            return { {}, ++m_acquires };
        }

        LifecycleResult Poll(const EntityAssetRequest&, Rtti::TypeID) override
        {
            return m_status;
        }

        void Release(EntityAssetRequest& request) override
        {
            ++m_releases;
            request.m_serviceToken = 0;
        }
    };


    const Uuid kRoot("8a1717a1-1a69-42b0-a79f-3226cf550001");
    const Uuid kChild("8a1717a1-1a69-42b0-a79f-3226cf550002");
    const Uuid kOther("8a1717a1-1a69-42b0-a79f-3226cf550003");
    const Uuid kAsset("8a1717a1-1a69-42b0-a79f-3226cf550004");

    struct WorldFixture : testing::Test
    {
        FakeAssets m_assets;
        EntityWorld m_world{ &m_assets };
        EntityRegistry& m_registry = m_world.CreateRegistry();

        void SetUp() override
        {
            Hook::s_trace.clear();
            Hook::s_pending = Hook::s_failStage = 0;
            CustomLoad::s_unloads = 0;
        }

        Entity& Spawn(Uuid uuid = kRoot, int32_t value = 7)
        {
            EntityCommandList commands(m_world);
            auto token = commands.CreateEntity(m_registry, "entity", uuid);
            commands.AddComponent(token, Number{ value });
            m_world.Submit(std::move(commands));
            EXPECT_TRUE(m_world.CommitBootstrap());
            return *m_world.Find(uuid);
        }

        template<class... Terms, class Function>
        uint32_t Count(Function&& function)
        {
            m_world.BeginUpdate();
            EntityUpdateContext context{ m_world, nullptr, m_world.GetEpoch() };
            uint32_t count = 0;
            auto group =
                Query<Terms...>::Create().Traverse(context,
                                                   Phases::Update,
                                                   [&](typename FE::Framework::Internal::QueryTerm<Terms>::Argument... args) {
                                                       ++count;
                                                       function(args...);
                                                   });
            EXPECT_FALSE(group->IsSignaled());
            EXPECT_EQ(count, 0);
            EXPECT_TRUE(m_world.SchedulePhase(Phases::Update));
            EXPECT_TRUE(m_world.ExecuteSchedule());
            EXPECT_TRUE(group->IsSignaled());
            EXPECT_TRUE(m_world.EndUpdate());
            return count;
        }


        template<class TQuery, class TFunction>
        void Traverse(TQuery query, TFunction&& function)
        {
            m_world.BeginUpdate();
            EntityUpdateContext context{ m_world, nullptr, m_world.GetEpoch() };
            query.Traverse(context, Phases::Update, std::forward<TFunction>(function));
            EXPECT_TRUE(m_world.SchedulePhase(Phases::Update));
            EXPECT_TRUE(m_world.ExecuteSchedule());
            EXPECT_TRUE(m_world.EndUpdate());
        }
    };
} // namespace


TEST_F(WorldFixture, NewEntityFiltersConsumePublicationIndependentlyAndIgnoreReactivation)
{
    Entity& root = Spawn();
    ChangeCursor first;
    ChangeCursor second;
    auto count = [&](ChangeCursor& cursor) {
        uint32_t count = 0;
        Traverse(Query<>::Create().SetChangeCursor(cursor).SetFilter(QueryFilter::kNewEntities), [&] {
            ++count;
        });
        return count;
    };
    EXPECT_EQ(count(first), 1);
    EXPECT_EQ(count(first), 0);
    EXPECT_EQ(count(second), 1);

    EntityCommandList edits(m_world);
    edits.ReplaceComponent(root.GetID(), Number{ 8 });
    edits.SetActive(root.GetID(), false);
    m_world.Submit(std::move(edits));
    EXPECT_EQ(count(first), 0);

    EntityCommandList reactivate(m_world);
    reactivate.SetActive(root.GetID(), true);
    auto child = reactivate.CreateEntity(m_registry, {}, kChild);
    reactivate.SetActive(child, false);
    m_world.Submit(std::move(reactivate));
    EXPECT_EQ(count(first), 0);

    EntityCommandList publish(m_world);
    publish.SetActive(m_world.Find(kChild, false)->GetID(), true);
    m_world.Submit(std::move(publish));
    EXPECT_EQ(count(first), 1);
    EXPECT_EQ(count(first), 0);
    EXPECT_EQ(count(second), 1);
}


TEST_F(WorldFixture, AddedComponentFiltersPreservePerRowHistoryAcrossMigrationAndCompaction)
{
    Entity& root = Spawn();
    Spawn(kChild);
    Spawn(kOther);
    ChangeCursor numbers;
    ChangeCursor extras;
    auto countNumbers = [&] {
        uint32_t count = 0;
        Traverse(Query<const Number>::Create().SetChangeCursor(numbers).SetFilter(QueryFilter::kAddedComponents),
                 [&](const Number&) {
                     ++count;
                 });
        return count;
    };
    auto countExtras = [&] {
        uint32_t count = 0;
        Traverse(Query<const Extra>::Create().SetChangeCursor(extras).SetFilter(QueryFilter::kAddedComponents),
                 [&](const Extra&) {
                     ++count;
                 });
        return count;
    };
    EXPECT_EQ(countNumbers(), 3);
    EXPECT_EQ(countExtras(), 0);

    EntityCommandList migrate(m_world);
    migrate.AddComponent(root.GetID(), Extra{ 1 });
    migrate.Destroy(m_world.Find(kChild)->GetID());
    migrate.ReplaceComponent(m_world.Find(kOther)->GetID(), Number{ 9 });
    m_world.Submit(std::move(migrate));
    EXPECT_EQ(countNumbers(), 0);
    EXPECT_EQ(countExtras(), 1);
    EXPECT_EQ(countExtras(), 0);

    EntityCommandList remove(m_world);
    remove.RemoveComponent<Extra>(root.GetID());
    m_world.Submit(std::move(remove));
    EXPECT_EQ(countExtras(), 0);

    EntityCommandList add(m_world);
    add.AddComponent(root.GetID(), Extra{ 2 });
    m_world.Submit(std::move(add));
    EXPECT_EQ(countExtras(), 1);
    EXPECT_EQ(countNumbers(), 0);

    EntityCommandList deactivate(m_world);
    deactivate.SetActive(root.GetID(), false);
    m_world.Submit(std::move(deactivate));
    EXPECT_EQ(countExtras(), 0);

    EntityCommandList reactivate(m_world);
    reactivate.SetActive(root.GetID(), true);
    m_world.Submit(std::move(reactivate));
    EXPECT_EQ(countExtras(), 0);
    EXPECT_EQ(countNumbers(), 0);
}


TEST_F(WorldFixture, AdditionFiltersWaitForAssetReadinessAndSuccessfulActivation)
{
    Entity& root = Spawn();
    ChangeCursor entities;
    ChangeCursor components;
    auto countEntities = [&] {
        uint32_t count = 0;
        Traverse(Query<const Number>::Create().SetChangeCursor(entities).SetFilter(QueryFilter::kNewEntities),
                 [&](const Number&) {
                     ++count;
                 });
        return count;
    };
    auto countComponents = [&] {
        uint32_t count = 0;
        Traverse(Query<const AssetComponent>::Create().SetChangeCursor(components).SetFilter(QueryFilter::kAddedComponents),
                 [&](const AssetComponent&) {
                     ++count;
                 });
        return count;
    };
    EXPECT_EQ(countEntities(), 1);
    EXPECT_EQ(countComponents(), 0);

    EntityCommandList commands(m_world);
    auto child = commands.CreateEntity(m_registry, {}, kChild);
    commands.AddComponent(child, Number{ 3 });
    commands.AddComponent(child, AssetComponent{ IO::Link<Number>(kAsset), {} });
    commands.AddComponent(root.GetID(), AssetComponent{ IO::Link<Number>(kAsset), {} });
    m_world.Submit(std::move(commands));
    EXPECT_EQ(countEntities(), 0);
    EXPECT_EQ(countComponents(), 0);

    m_assets.m_status = LifecycleResult::kSucceeded;
    EXPECT_EQ(countEntities(), 1);
    EXPECT_EQ(countComponents(), 2);
    EXPECT_EQ(countComponents(), 0);

    Hook::s_failStage = 3;
    EntityCommandList failed(m_world);
    failed.AddComponent(root.GetID(), Hook{ 1 });
    m_world.Submit(std::move(failed));
    ChangeCursor failedCursor;
    uint32_t failedCount = 0;
    Traverse(Query<const Hook>::Create().SetChangeCursor(failedCursor).SetFilter(QueryFilter::kAddedComponents),
             [&](const Hook&) {
                 ++failedCount;
             });
    EXPECT_EQ(failedCount, 0);
    Hook::s_failStage = 0;
}


TEST_F(WorldFixture, AddedComponentFiltersSupportOptionalTermsParentsAndParallelCascades)
{
    EntityCommandList commands(m_world);
    const auto root = commands.CreateEntity(m_registry, {}, kRoot);
    commands.AddComponent(root, Number{ 1 });
    for (uint32_t index = 0; index < 80; ++index)
    {
        auto child = commands.CreateEntity(m_registry);
        commands.AddComponent(child, Number{ 2 });
        commands.SetParent(child, root);
    }

    m_world.Submit(std::move(commands));
    ASSERT_TRUE(m_world.CommitBootstrap());
    ChangeCursor cursor;
    auto count = [&] {
        std::atomic<uint32_t> count = 0;
        using Query = CascadeQuery<const Number, const Extra*, Parent<const Number*>, Without<Hook>>;
        Traverse(Query::Create()
                     .SetChangeCursor(cursor)
                     .SetFilter(QueryFilter::kAddedComponents)
                     .SetExecutionPolicy(ExecutionPolicy::kParallel),
                 [&](const Number&, const Extra*, const Number*) {
                     count.fetch_add(1, std::memory_order_relaxed);
                 });
        return count.load();
    };
    EXPECT_EQ(count(), 81);
    EXPECT_EQ(count(), 0);

    EntityCommandList add(m_world);
    add.AddComponent(m_world.Find(kRoot)->GetID(), Extra{ 1 });
    m_world.Submit(std::move(add));
    EXPECT_EQ(count(), 1);
    EXPECT_EQ(count(), 0);

    EntityCommandList replace(m_world);
    replace.ReplaceComponent(m_world.Find(kRoot)->GetID(), Number{ 3 });
    m_world.Submit(std::move(replace));
    EXPECT_EQ(count(), 0);
}


TEST_F(WorldFixture, AdditionQueriesOrderInitializationThroughTraversalPrerequisites)
{
    Entity& entity = Spawn();
    ChangeCursor prepareCursor;
    ChangeCursor finishCursor;
    uint32_t prepared = 0;
    uint32_t finished = 0;
    auto tick = [&] {
        m_world.BeginUpdate();
        EntityUpdateContext context{ m_world, nullptr, m_world.GetEpoch() };
        auto prepare = Query<Number, const Extra>::Create()
                           .SetFilter(QueryFilter::kAddedComponents)
                           .SetChangeCursor(prepareCursor)
                           .Traverse(context, Phases::Update, [&](Number& number, const Extra& extra) {
                               number.m_value = extra.m_value * 2;
                               ++prepared;
                           });
        Query<const Number, Extra>::Create()
            .SetFilter(QueryFilter::kAddedComponents)
            .SetChangeCursor(finishCursor)
            .SetPrerequisites({ prepare })
            .Traverse(context, Phases::Update, [&](const Number& number, Extra& extra) {
                extra.m_value = number.m_value + 1;
                ++finished;
            });
        EXPECT_TRUE(m_world.SchedulePhase(Phases::Update));
        EXPECT_TRUE(m_world.ExecuteSchedule());
        EXPECT_TRUE(m_world.EndUpdate());
    };
    tick();
    EXPECT_EQ(prepared, 0);
    EXPECT_EQ(finished, 0);

    EntityCommandList add(m_world);
    add.AddComponent(entity.GetID(), Extra{ 4 });
    m_world.Submit(std::move(add));
    tick();
    EXPECT_EQ(prepared, 1);
    EXPECT_EQ(finished, 1);
    EXPECT_EQ(entity.FindComponent<const Number>()->m_value, 8);
    EXPECT_EQ(entity.FindComponent<const Extra>()->m_value, 9);
    tick();
    EXPECT_EQ(prepared, 1);
    EXPECT_EQ(finished, 1);
}


TEST_F(WorldFixture, IdentityMigrationAndMoveOnlyCompaction)
{
    EXPECT_EQ(MoveOnly::s_live, 0);
    EntityCommandList commands(m_world);
    auto a = commands.CreateEntity(m_registry, "a", kRoot);
    auto b = commands.CreateEntity(m_registry, "b", kChild);
    commands.AddComponent(a, MoveOnly{ 11 });
    commands.AddComponent(b, MoveOnly{ 22 });
    m_world.Submit(std::move(commands));
    ASSERT_TRUE(m_world.CommitBootstrap());
    Entity* stable = m_world.Find(kChild);
    const EntityID id = stable->GetID();
    EntityCommandList migration(m_world);
    migration.AddComponent(id, Extra{ 31 });
    migration.Destroy(m_world.Find(kRoot)->GetID());
    m_world.Submit(std::move(migration));
    ASSERT_TRUE(m_world.CommitBootstrap());
    ASSERT_EQ(m_world.Find(id), stable);
    EXPECT_EQ(stable->GetUuid(), kChild);
    EXPECT_EQ(stable->FindComponent<MoveOnly>()->m_value, 22);
    EXPECT_EQ(MoveOnly::s_live, 1);
    EntityCommandList destroy(m_world);
    destroy.Destroy(id);
    m_world.Submit(std::move(destroy));
    ASSERT_TRUE(m_world.CommitBootstrap());
    EXPECT_EQ(m_world.Find(id), nullptr);
    EXPECT_EQ(MoveOnly::s_live, 0);
    EXPECT_EQ(m_world.GetChunkCount(), 0);
    Entity& replacement = Spawn(kOther);
    EXPECT_NE(replacement.GetID().m_value, id.m_value);
    EXPECT_EQ(m_world.Find(id), nullptr);
}


TEST_F(WorldFixture, OverAlignedAndOversizedComponents)
{
    EntityCommandList commands(m_world);
    auto token = commands.CreateEntity(m_registry, "big", kRoot);
    ASSERT_TRUE(commands.AddComponent(token, Aligned{}));
    Huge huge;
    huge.m_data[69999] = 81;
    ASSERT_TRUE(commands.AddComponent(token, std::move(huge)));
    m_world.Submit(std::move(commands));
    ASSERT_TRUE(m_world.CommitBootstrap());
    auto* entity = m_world.Find(kRoot);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(entity->FindComponent<Aligned>()) % 256, 0);
    EXPECT_EQ(entity->FindComponent<Huge>()->m_data[69999], 81);
}


TEST_F(WorldFixture, HandlesCannotCrossWorldOrWorldIncarnations)
{
    EntityID old;
    {
        EntityWorld other;
        auto& registry = other.CreateRegistry();
        EntityCommandList commands(other);
        commands.CreateEntity(registry, {}, kRoot);
        other.Submit(std::move(commands));
        ASSERT_TRUE(other.CommitBootstrap());
        old = other.Find(kRoot)->GetID();
        EXPECT_EQ(m_world.Find(old), nullptr);
    }
    Entity& current = Spawn();
    EXPECT_EQ(m_world.Find(old), nullptr);
    EXPECT_NE(old.World(), current.GetID().World());
}


TEST_F(WorldFixture, NextFrameGateCannotBeBypassedWithRepeatedCommit)
{
    m_world.BeginUpdate();
    EntityCommandList commands(m_world);
    auto token = commands.CreateEntity(m_registry, {}, kRoot);
    commands.AddComponent(token, Number{ 19 });
    m_world.Submit(std::move(commands));
    ASSERT_TRUE(m_world.Commit());
    ASSERT_TRUE(m_world.Commit());
    EXPECT_EQ(m_world.Find(kRoot, false), nullptr);
    EXPECT_DEATH(m_world.CommitBootstrap(), "");
    ASSERT_TRUE(m_world.EndUpdate());
    m_world.BeginUpdate();
    ASSERT_NE(m_world.Find(kRoot), nullptr);
    EXPECT_EQ(m_world.Find(kRoot)->FindComponent<Number>()->m_value, 19);
    ASSERT_TRUE(m_world.EndUpdate());
}


TEST_F(WorldFixture, HierarchyValidationIsAtomicAndSiblingOrderStable)
{
    EntityCommandList commands(m_world);
    auto root = commands.CreateEntity(m_registry, {}, kRoot);
    auto child = commands.CreateEntity(m_registry, {}, kChild);
    auto other = commands.CreateEntity(m_registry, {}, kOther);
    commands.SetParent(child, root);
    commands.SetParent(other, root);
    m_world.Submit(std::move(commands));
    ASSERT_TRUE(m_world.CommitBootstrap());
    Entity* r = m_world.Find(kRoot);
    Entity* c = m_world.Find(kChild);
    Entity* o = m_world.Find(kOther);
    ASSERT_EQ(r->GetFirstChild(), c);
    ASSERT_EQ(c->GetNextSibling(), o);
    EntityCommandList invalid(m_world);
    invalid.Rename(r->GetID(), "changed");
    invalid.SetParent(r->GetID(), c->GetID());
    m_world.Submit(std::move(invalid));
    EXPECT_FALSE(m_world.CommitBootstrap());
    EXPECT_FALSE(r->GetName().IsValid());
    EXPECT_EQ(r->GetParent(), nullptr);
    EXPECT_EQ(r->GetFirstChild(), c);
    EntityCommandList rename(m_world);
    rename.Rename(c->GetID(), "child");
    m_world.Submit(std::move(rename));
    ASSERT_TRUE(m_world.CommitBootstrap());
    EXPECT_EQ(r->GetFirstChild(), c);
    EXPECT_EQ(c->GetNextSibling(), o);
    EntityCommandList destroy(m_world);
    destroy.Destroy(r->GetID());
    m_world.Submit(std::move(destroy));
    ASSERT_TRUE(m_world.CommitBootstrap());
    EXPECT_EQ(m_world.GetEntityCount(), 0);
    EXPECT_EQ(m_world.Find(kChild, false), nullptr);
    EXPECT_EQ(m_world.GetChunkCount(), 0);
}


TEST_F(WorldFixture, InvalidRegistryParentAndForeignTokensAreRejected)
{
    Entity& root = Spawn();
    auto& otherRegistry = m_world.CreateRegistry();
    EntityCommandList commands(m_world);
    auto other = commands.CreateEntity(otherRegistry, {}, kOther);
    commands.SetParent(other, root.GetID());
    m_world.Submit(std::move(commands));
    EXPECT_FALSE(m_world.CommitBootstrap());
    EXPECT_EQ(m_world.Find(kOther, false), nullptr);
    EntityCommandList first(m_world), second(m_world);
    auto token = first.CreateEntity(m_registry, {}, kChild);
    EXPECT_DEBUG_DEATH(second.Rename(token, "invalid"), "");
}


TEST_F(WorldFixture, StaleCommandsAndIndependentConflictsDoNotModifyEntities)
{
    Entity& root = Spawn();
    EntityCommandList a(m_world), b(m_world);
    a.Rename(root.GetID(), "a");
    b.Rename(root.GetID(), "b");
    m_world.Submit(std::move(a));
    m_world.Submit(std::move(b));
    EXPECT_FALSE(m_world.CommitBootstrap());
    EXPECT_EQ(root.GetName(), Env::Name("entity"));
    EntityID id = root.GetID();
    EntityCommandList destroy(m_world);
    destroy.Destroy(id);
    destroy.Rename(id, "ignored");
    m_world.Submit(std::move(destroy));
    ASSERT_TRUE(m_world.CommitBootstrap());
    EntityCommandList stale(m_world);
    stale.Rename(id, "stale");
    m_world.Submit(std::move(stale));
    EXPECT_FALSE(m_world.CommitBootstrap());
}


TEST_F(WorldFixture, HooksInitializeChildrenFirstActivateParentsFirstAndTearDownInReverse)
{
    EntityCommandList commands(m_world);
    auto root = commands.CreateEntity(m_registry, {}, kRoot);
    auto child = commands.CreateEntity(m_registry, {}, kChild);
    commands.AddComponent(root, Hook{ 1 });
    commands.AddComponent(child, Hook{ 2 });
    commands.SetParent(child, root);
    m_world.Submit(std::move(commands));
    ASSERT_TRUE(m_world.CommitBootstrap());
    EXPECT_EQ(Hook::s_trace, (festd::vector<int32_t>{ 11, 21, 22, 12, 13, 23 }));
    Hook::s_trace.clear();
    EntityCommandList destroy(m_world);
    destroy.Destroy(m_world.Find(kRoot)->GetID());
    m_world.Submit(std::move(destroy));
    ASSERT_TRUE(m_world.CommitBootstrap());
    EXPECT_EQ(Hook::s_trace, (festd::vector<int32_t>{ 24, 25, 26, 14, 15, 16 }));
}


TEST_F(WorldFixture, SameLayoutReplacementPreservesRetainedComponentStorage)
{
    Entity& entity = Spawn();
    EntityCommandList add(m_world);
    add.AddComponent(entity.GetID(), MoveOnly{ 19 });
    m_world.Submit(std::move(add));
    ASSERT_TRUE(m_world.CommitBootstrap());
    const auto* retained = entity.FindComponent<const MoveOnly>();
    const int32_t moves = MoveOnly::s_moves;
    const uint32_t chunks = m_world.GetChunkCount();

    EntityCommandList replace(m_world);
    replace.ReplaceComponent(entity.GetID(), Number{ 42 });
    m_world.Submit(std::move(replace));
    ASSERT_TRUE(m_world.CommitBootstrap());
    EXPECT_EQ(entity.FindComponent<const MoveOnly>(), retained);
    EXPECT_EQ(MoveOnly::s_moves, moves);
    EXPECT_EQ(m_world.GetChunkCount(), chunks);
    EXPECT_EQ(entity.FindComponent<const Number>()->m_value, 42);
}


TEST_F(WorldFixture, SparseTransactionDestroysSubtreeMovedUnderItsTarget)
{
    EntityCommandList create(m_world);
    create.CreateEntity(m_registry, {}, kRoot);
    auto other = create.CreateEntity(m_registry, {}, kOther);
    auto child = create.CreateEntity(m_registry, {}, kChild);
    create.SetParent(child, other);
    m_world.Submit(std::move(create));
    ASSERT_TRUE(m_world.CommitBootstrap());

    EntityCommandList edit(m_world);
    edit.SetParent(m_world.Find(kOther)->GetID(), m_world.Find(kRoot)->GetID());
    edit.Destroy(m_world.Find(kRoot)->GetID());
    m_world.Submit(std::move(edit));
    ASSERT_TRUE(m_world.CommitBootstrap());
    EXPECT_EQ(m_world.GetEntityCount(), 0);
}


TEST_F(WorldFixture, DeepHierarchyFailureReleasesEachLoadedComponentOnce)
{
    constexpr uint32_t kDepth = 128;
    EntityCommandList commands(m_world);
    EntityToken parent;
    for (uint32_t i = 0; i < kDepth; ++i)
    {
        const auto token = commands.CreateEntity(m_registry);
        commands.AddComponent(token, Hook{ static_cast<int32_t>(i + 1) });
        if (i != 0)
            commands.SetParent(token, parent);

        parent = token;
    }
    Hook::s_failStage = 3;
    m_world.Submit(std::move(commands));
    ASSERT_TRUE(m_world.CommitBootstrap());
    for (uint32_t i = 0; i < kDepth; ++i)
    {
        const int32_t unloaded = static_cast<int32_t>((i + 1) * 10 + 6);
        EXPECT_EQ(std::count(Hook::s_trace.begin(), Hook::s_trace.end(), unloaded), 1);
    }
    const auto trace = Hook::s_trace;
    ASSERT_TRUE(m_world.CommitBootstrap());
    EXPECT_EQ(Hook::s_trace, trace);
    m_world.Clear();
    EXPECT_EQ(Hook::s_trace, trace);
    Hook::s_failStage = 0;
}


TEST_F(WorldFixture, PendingAdditionLeavesExistingActiveComponentQueryable)
{
    Entity& entity = Spawn();
    Hook::s_pending = 1;
    EntityCommandList commands(m_world);
    commands.AddComponent(entity.GetID(), Hook{ 1 });
    m_world.Submit(std::move(commands));
    ASSERT_TRUE(m_world.CommitBootstrap());
    ASSERT_TRUE(entity.IsActive());
    EXPECT_EQ((Count<const Number, const Hook*>([](const Number& number, const Hook* hook) {
                  EXPECT_EQ(number.m_value, 7);
                  EXPECT_EQ(hook, nullptr);
              })),
              1);
    Hook::s_pending = 0;
    EXPECT_EQ((Count<const Number, const Hook*>([](const Number&, const Hook* hook) {
                  EXPECT_NE(hook, nullptr);
              })),
              1);
}


TEST_F(WorldFixture, FailureAtEveryStageUnwindsExactlyOnce)
{
    for (int32_t failure = 1; failure <= 3; ++failure)
    {
        Hook::s_trace.clear();
        Hook::s_failStage = failure;
        EntityCommandList commands(m_world);
        auto token = commands.CreateEntity(m_registry, {}, kRoot);
        commands.AddComponent(token, Hook{ 1 });
        m_world.Submit(std::move(commands));
        ASSERT_TRUE(m_world.CommitBootstrap());
        Entity* entity = m_world.Find(kRoot, false);
        ASSERT_NE(entity, nullptr);
        EXPECT_TRUE(entity->HasFailed());
        EXPECT_FALSE(entity->IsActive());
        const auto expected = failure == 1 ? festd::vector<int32_t>{ 11, 16 }
            : failure == 2                 ? festd::vector<int32_t>{ 11, 12, 15, 16 }
                                           : festd::vector<int32_t>{ 11, 12, 13, 14, 15, 16 };
        EXPECT_EQ(Hook::s_trace, expected);
        EntityCommandList destroy(m_world);
        destroy.Destroy(entity->GetID());
        m_world.Submit(std::move(destroy));
        ASSERT_TRUE(m_world.CommitBootstrap());
        EXPECT_EQ(Hook::s_trace, expected);
    }
    Hook::s_failStage = 0;
}


TEST_F(WorldFixture, SerializedHardDependenciesAreSharedAndSoftDependenciesAreIgnored)
{
    EntityCommandList commands(m_world);
    for (const Uuid uuid : { kRoot, kChild })
    {
        auto token = commands.CreateEntity(m_registry, {}, uuid, ResidencyScope::kRegistry);
        commands.AddComponent(token,
                              AssetComponent{ IO::Link<Number>(kAsset), IO::Link<Number, IO::DependencyKind::kSoft>(kOther) });
    }
    m_world.Submit(std::move(commands));
    ASSERT_TRUE(m_world.CommitBootstrap());
    EXPECT_EQ(m_assets.m_acquires, 1);
    EXPECT_EQ(m_world.Find(kRoot), nullptr);
    m_assets.m_status = LifecycleResult::kSucceeded;
    ASSERT_TRUE(m_world.CommitBootstrap());
    ASSERT_NE(m_world.Find(kRoot), nullptr);
    EntityCommandList remove(m_world);
    remove.RemoveComponent<AssetComponent>(m_world.Find(kRoot)->GetID());
    m_world.Submit(std::move(remove));
    ASSERT_TRUE(m_world.CommitBootstrap());
    EXPECT_EQ(m_assets.m_releases, 0);
    EntityCommandList destroy(m_world);
    destroy.Destroy(m_world.Find(kChild)->GetID());
    m_world.Submit(std::move(destroy));
    ASSERT_TRUE(m_world.CommitBootstrap());
    EXPECT_EQ(m_assets.m_releases, 1);
}


TEST_F(WorldFixture, CancelPendingLoadPreventsLatePublicationAndReleasesRequests)
{
    EntityCommandList commands(m_world);
    auto token = commands.CreateEntity(m_registry, {}, kRoot);
    commands.AddComponent(token, CustomLoad{ kAsset });
    m_world.Submit(std::move(commands));
    ASSERT_TRUE(m_world.CommitBootstrap());
    EXPECT_EQ(m_assets.m_acquires, 1);
    EXPECT_EQ(m_world.Find(kRoot), nullptr);
    EntityCommandList destroy(m_world);
    destroy.Destroy(m_world.Find(kRoot, false)->GetID());
    m_world.Submit(std::move(destroy));
    ASSERT_TRUE(m_world.CommitBootstrap());
    EXPECT_EQ(CustomLoad::s_unloads, 1);
    EXPECT_EQ(m_assets.m_releases, 1);
    m_assets.m_status = LifecycleResult::kSucceeded;
    ASSERT_TRUE(m_world.CommitBootstrap());
    EXPECT_EQ(m_world.Find(kRoot, false), nullptr);
}


TEST_F(WorldFixture, DeactivationRetainsInitializationAndReactivationDoesNotReload)
{
    EntityCommandList commands(m_world);
    auto token = commands.CreateEntity(m_registry, {}, kRoot);
    commands.AddComponent(token, Hook{ 1 });
    m_world.Submit(std::move(commands));
    ASSERT_TRUE(m_world.CommitBootstrap());
    Hook::s_trace.clear();
    Entity* entity = m_world.Find(kRoot);
    EntityCommandList deactivate(m_world);
    deactivate.SetActive(entity->GetID(), false);
    m_world.Submit(std::move(deactivate));
    ASSERT_TRUE(m_world.CommitBootstrap());
    EXPECT_FALSE(entity->IsActive());
    EntityCommandList reactivate(m_world);
    reactivate.SetActive(entity->GetID(), true);
    m_world.Submit(std::move(reactivate));
    ASSERT_TRUE(m_world.CommitBootstrap());
    EXPECT_TRUE(entity->IsActive());
    EXPECT_EQ(Hook::s_trace, (festd::vector<int32_t>{ 14, 13 }));
}


TEST_F(WorldFixture, QueriesIncludeEveryRegistryAndOptionalEntityArgument)
{
    Spawn();
    auto& registry = m_world.CreateRegistry();
    EntityCommandList commands(m_world);
    auto token = commands.CreateEntity(registry, {}, kOther);
    commands.AddComponent(token, Number{ 9 });
    commands.AddComponent(token, Extra{ 3 });
    m_world.Submit(std::move(commands));
    ASSERT_TRUE(m_world.CommitBootstrap());
    m_world.BeginUpdate();
    EntityUpdateContext context{ m_world, nullptr, m_world.GetEpoch() };
    int32_t sum = 0;
    auto group = Query<Number, const Extra*>::Create().Traverse(context,
                                                                Phases::Update,
                                                                [&](Entity& entity, Number& number, const Extra* extra) {
                                                                    EXPECT_EQ(entity.FindComponent<Number>(), &number);
                                                                    sum += number.m_value + (extra ? extra->m_value : 0);
                                                                    ++number.m_value;
                                                                });
    EXPECT_EQ(sum, 0);
    ASSERT_TRUE(m_world.SchedulePhase(Phases::Update));
    ASSERT_TRUE(m_world.ExecuteSchedule());
    EXPECT_EQ(sum, 19);
    ASSERT_TRUE(m_world.EndUpdate());
    m_world.RemoveRegistry(registry);
    EXPECT_EQ((Count<const Number>([](const Number& number) {
                  EXPECT_EQ(number.m_value, 8);
              })),
              1);
}


TEST_F(WorldFixture, WithoutFiltersHaveNoArgumentsOrDataAccess)
{
    Spawn();
    EntityCommandList commands(m_world);
    auto child = commands.CreateEntity(m_registry, {}, kChild);
    commands.AddComponent(child, Number{ 9 });
    commands.AddComponent(child, Extra{ 3 });
    auto other = commands.CreateEntity(m_registry, {}, kOther);
    commands.AddComponent(other, Number{ 11 });
    commands.AddComponent(other, Hook{ 1 });
    m_world.Submit(std::move(commands));
    ASSERT_TRUE(m_world.CommitBootstrap());

    m_world.BeginUpdate();
    EntityUpdateContext context{ m_world, nullptr, m_world.GetEpoch() };
    uint32_t filtered = 0;
    uint32_t filterOnly = 0;
    uint32_t optional = 0;
    Query<Without<Extra>, const Number, Without<Hook>>::Create().Traverse(context,
                                                                          Phases::Update,
                                                                          [&](Entity& entity, const Number& number) {
                                                                              EXPECT_EQ(entity.GetUuid(), kRoot);
                                                                              EXPECT_EQ(number.m_value, 7);
                                                                              ++filtered;
                                                                          });
    Query<Without<Extra>>::Create().Traverse(context, Phases::Update, [&] {
        ++filterOnly;
    });
    Query<const Number*, Without<Extra>>::Create().Traverse(context, Phases::Update, [&](const Number* number) {
        EXPECT_NE(number, nullptr);
        ++optional;
    });
    Query<Extra>::Create().Traverse(context, Phases::Update, [](Extra& extra) {
        ++extra.m_value;
    });
    ASSERT_TRUE(m_world.SchedulePhase(Phases::Update));
    ASSERT_TRUE(m_world.ExecuteSchedule());
    EXPECT_EQ(filtered, 1);
    EXPECT_EQ(filterOnly, 2);
    EXPECT_EQ(optional, 2);
    EXPECT_EQ(m_world.GetScheduleDiagnostics().m_conflictEdges, 0);
    ASSERT_TRUE(m_world.EndUpdate());
}


TEST_F(WorldFixture, WithoutFiltersObserveComponentAdditionAndRemoval)
{
    Entity& entity = Spawn();
    ChangeCursor changes;
    auto count = [&] {
        m_world.BeginUpdate();
        EntityUpdateContext context{ m_world, nullptr, m_world.GetEpoch() };
        uint32_t matched = 0;
        Query<const Number, Without<Extra>>::Create().SetChangeCursor(changes).Traverse(context,
                                                                                        Phases::Update,
                                                                                        [&](const Number&) {
                                                                                            ++matched;
                                                                                        });
        EXPECT_TRUE(m_world.SchedulePhase(Phases::Update));
        EXPECT_TRUE(m_world.ExecuteSchedule());
        EXPECT_TRUE(m_world.EndUpdate());
        return matched;
    };
    EXPECT_EQ(count(), 1);
    EXPECT_EQ(count(), 0);

    EntityCommandList add(m_world);
    add.AddComponent(entity.GetID(), Extra{ 3 });
    m_world.Submit(std::move(add));
    EXPECT_EQ(count(), 0);

    EntityCommandList remove(m_world);
    remove.RemoveComponent<Extra>(entity.GetID());
    m_world.Submit(std::move(remove));
    EXPECT_EQ(count(), 1);
    EXPECT_EQ(count(), 0);
}


TEST_F(WorldFixture, WithoutSelfComponentAllowsTheSameParentComponent)
{
    Spawn();
    EntityCommandList commands(m_world);
    auto child = commands.CreateEntity(m_registry, {}, kChild);
    commands.AddComponent(child, Extra{ 3 });
    commands.SetParent(child, m_world.Find(kRoot)->GetID());
    m_world.Submit(std::move(commands));
    ASSERT_TRUE(m_world.CommitBootstrap());

    m_world.BeginUpdate();
    EntityUpdateContext context{ m_world, nullptr, m_world.GetEpoch() };
    uint32_t matched = 0;
    CascadeQuery<Without<Number>, Parent<const Number>, const Extra>::Create().Traverse(
        context,
        Phases::Update,
        [&](const Number& parent, const Extra& extra) {
            EXPECT_EQ(parent.m_value, 7);
            EXPECT_EQ(extra.m_value, 3);
            ++matched;
        });
    ASSERT_TRUE(m_world.SchedulePhase(Phases::Update));
    ASSERT_TRUE(m_world.ExecuteSchedule());
    EXPECT_EQ(matched, 1);
    ASSERT_TRUE(m_world.EndUpdate());
}


TEST_F(WorldFixture, ExplicitPrerequisitesOverrideCollectionOrderAndEmptyWorkCompletes)
{
    Entity& entity = Spawn();
    m_world.BeginUpdate();
    EntityUpdateContext context{ m_world, nullptr, m_world.GetEpoch() };
    auto reset = Query<Number>::Create().Traverse(context, Phases::PreUpdate, [](Number& n) {
        n.m_value = 1;
    });
    auto add = Query<Number>::Create().SetPrerequisites({ reset }).Traverse(context, Phases::PostUpdate, [](Number& n) {
        n.m_value += 4;
    });
    auto empty = Query<const Extra>::Create().SetPrerequisites({ add }).Traverse(context, Phases::PostUpdate, [](const Extra&) {
        ADD_FAILURE();
    });
    ASSERT_TRUE(m_world.SchedulePhase(Phases::PreUpdate));
    ASSERT_TRUE(m_world.SchedulePhase(Phases::PostUpdate));
    ASSERT_TRUE(m_world.ExecuteSchedule());
    EXPECT_EQ(entity.FindComponent<Number>()->m_value, 5);
    EXPECT_TRUE(empty->IsSignaled());
    ASSERT_TRUE(m_world.EndUpdate());
}


TEST_F(WorldFixture, InvalidSchedulesAssertBeforeCallbacks)
{
    Spawn();
    for (int scenario = 0; scenario < 3; ++scenario)
    {
        EXPECT_DEATH(
            ([&] {
                m_world.BeginUpdate();
                EntityUpdateContext context{ m_world, nullptr, m_world.GetEpoch() };
                auto first = Query<Number>::Create().Traverse(context, Phases::PostUpdate, [](Number&) {
                    ADD_FAILURE();
                });
                if (scenario == 0)
                {
                    m_world.SchedulePhase(Phases::PreUpdate);
                }
                else if (scenario == 1)
                {
                    m_world.SchedulePhase(Phases::PostUpdate);
                    m_world.SchedulePhase(Phases::PostUpdate);
                }
                else
                {
                    Query<Number>::Create().SetPrerequisites({ first }).Traverse(context, Phases::PreUpdate, [](Number&) {
                        ADD_FAILURE();
                    });
                    m_world.SchedulePhase(Phases::PreUpdate);
                    m_world.SchedulePhase(Phases::PostUpdate);
                }

                m_world.ValidateSchedule();
            }()),
            "");
    }
}


TEST_F(WorldFixture, IncompleteUpdateScheduleAsserts)
{
    Spawn();
    m_world.BeginUpdate();
    EntityUpdateContext context{ m_world, nullptr, m_world.GetEpoch() };
    Query<Number>::Create().Traverse(context, Phases::Update, [](Number&) {
        ADD_FAILURE();
    });
    ExpectIncompleteUpdate(m_world);
}


TEST_F(WorldFixture, PhaseCannotDependOnWorkItHasNotScheduledYet)
{
    Spawn();
    m_world.BeginUpdate();
    EntityUpdateContext context{ m_world, nullptr, m_world.GetEpoch() };
    auto group = Query<Number>::Create().Traverse(context, Phases::Update, [](Number&) {
        ADD_FAILURE();
    });
    Rc<WaitGroup> prerequisite[] = { group };
    m_world.SchedulePhase(Phases::Update, prerequisite);
    EXPECT_DEATH(m_world.ValidateSchedule(), "");
    ExpectIncompleteUpdate(m_world);
}


TEST_F(WorldFixture, SystemsCollectMultipleTraversalsAndShutdownInReverseOrder)
{
    Spawn();
    struct System final : WorldSystem
    {
        festd::vector<int>& m_trace;
        int m_label;
        System(festd::vector<int>& trace, int label)
            : m_trace(trace)
            , m_label(label)
        {
        }


        void Init(EntityWorld&) override
        {
            m_trace.push_back(m_label);
        }


        void Shutdown(EntityWorld&) override
        {
            m_trace.push_back(-m_label);
        }


        void Update(EntityUpdateContext& context) override
        {
            Query<Number>::Create().Traverse(context, Phases::PostUpdate, [](Number& number) {
                ++number.m_value;
            });
            Query<Number>::Create().Traverse(context, Phases::PreUpdate, [](Number& number) {
                number.m_value *= 2;
            });
        }
    };
    festd::vector<int> trace;
    System first(trace, 1), second(trace, 2);
    m_world.AddSystem(first);
    m_world.AddSystem(second);
    m_world.BeginUpdate();
    m_world.SchedulePhase(Phases::PreUpdate);
    m_world.SchedulePhase(Phases::PostUpdate);
    ASSERT_TRUE(m_world.ExecuteSchedule());
    ASSERT_TRUE(m_world.EndUpdate());
    EXPECT_EQ(m_world.Find(kRoot)->FindComponent<Number>()->m_value, 30);
    m_world.RemoveSystem(second);
    m_world.RemoveSystem(first);
    EXPECT_EQ(trace, (festd::vector<int>{ 1, 2, -2, -1 }));
}


TEST_F(WorldFixture, FailedAdditionDoesNotHideOrPoisonExistingComponents)
{
    Entity& entity = Spawn();
    Hook::s_failStage = 2;
    EntityCommandList commands(m_world);
    commands.AddComponent(entity.GetID(), Hook{ 1 });
    m_world.Submit(std::move(commands));
    ASSERT_TRUE(m_world.CommitBootstrap());
    EXPECT_TRUE(entity.IsActive());
    EXPECT_FALSE(entity.HasFailed());
    EXPECT_EQ(Hook::s_trace, (festd::vector<int32_t>{ 11, 12, 15, 16 }));
    EXPECT_EQ((Count<const Number, const Hook*>([](const Number&, const Hook* hook) {
                  EXPECT_EQ(hook, nullptr);
              })),
              1);
    Hook::s_failStage = 0;
}


TEST_F(WorldFixture, ReplacementsRetainTheOldValueAndDependenciesUntilReady)
{
    m_assets.m_status = LifecycleResult::kSucceeded;
    EntityCommandList commands(m_world);
    auto token = commands.CreateEntity(m_registry, {}, kRoot);
    commands.AddComponent(token, AssetComponent{ IO::Link<Number>(kAsset), {} });
    m_world.Submit(std::move(commands));
    ASSERT_TRUE(m_world.CommitBootstrap());
    Entity& entity = *m_world.Find(kRoot);
    m_assets.m_status = LifecycleResult::kPending;
    EntityCommandList replacement(m_world);
    replacement.ReplaceComponent(entity.GetID(), AssetComponent{ IO::Link<Number>(kOther), {} });
    m_world.Submit(std::move(replacement));
    ASSERT_TRUE(m_world.CommitBootstrap());
    EXPECT_EQ(entity.FindComponent<AssetComponent>()->m_hard.GetAssetID(), kAsset);
    EXPECT_TRUE(entity.IsActive());
    EXPECT_EQ(m_assets.m_acquires, 2);
    EXPECT_EQ(m_assets.m_releases, 0);
    m_assets.m_status = LifecycleResult::kSucceeded;
    ASSERT_TRUE(m_world.CommitBootstrap());
    EXPECT_EQ(entity.FindComponent<AssetComponent>()->m_hard.GetAssetID(), kOther);
    EXPECT_EQ(m_assets.m_releases, 1);
}


TEST_F(WorldFixture, FailedReplacementRetainsPreviousActiveValue)
{
    Entity& entity = Spawn();
    EntityCommandList add(m_world);
    add.AddComponent(entity.GetID(), Hook{ 1 });
    m_world.Submit(std::move(add));
    ASSERT_TRUE(m_world.CommitBootstrap());
    Hook::s_trace.clear();
    Hook::s_failStage = 3;
    EntityCommandList replace(m_world);
    replace.ReplaceComponent(entity.GetID(), Hook{ 2 });
    m_world.Submit(std::move(replace));
    ASSERT_TRUE(m_world.CommitBootstrap());
    EXPECT_EQ(entity.FindComponent<Hook>()->m_label, 1);
    EXPECT_TRUE(entity.IsActive());
    EXPECT_FALSE(entity.HasFailed());
    EXPECT_EQ(Hook::s_trace, (festd::vector<int32_t>{ 21, 22, 23, 24, 25, 26 }));
    Hook::s_failStage = 0;
}


TEST_F(WorldFixture, ActiveChildCanPublishAddedComponentsWithoutReactivatingAncestors)
{
    EntityCommandList commands(m_world);
    auto root = commands.CreateEntity(m_registry, {}, kRoot);
    auto child = commands.CreateEntity(m_registry, {}, kChild);
    commands.AddComponent(root, Hook{ 1 });
    commands.AddComponent(child, Number{ 7 });
    commands.SetParent(child, root);
    m_world.Submit(std::move(commands));
    ASSERT_TRUE(m_world.CommitBootstrap());
    Hook::s_trace.clear();
    EntityCommandList add(m_world);
    add.AddComponent(m_world.Find(kChild)->GetID(), Hook{ 2 });
    m_world.Submit(std::move(add));
    ASSERT_TRUE(m_world.CommitBootstrap());
    EXPECT_EQ(Hook::s_trace, (festd::vector<int32_t>{ 21, 22, 23 }));
    EXPECT_EQ((Count<const Hook>([](const Hook&) {})), 2);
}


TEST_F(WorldFixture, ApplicationStagesAndSamePhaseForwardDependenciesExecuteInDeclaredOrder)
{
    Entity& entity = Spawn();
    m_world.BeginUpdate();
    EntityUpdateContext context{ m_world, nullptr, m_world.GetEpoch() };
    auto later = Query<Number>::Create().Traverse(context, Phases::PostUpdate, [](Number& n) {
        n.m_value += 5;
    });
    auto earlier = Query<Number>::Create().Traverse(context, Phases::PostUpdate, [](Number& n) {
        n.m_value *= 3;
    });
    ASSERT_TRUE(m_world.AddPrerequisite(*later, earlier));
    constexpr Phase ApplicationStage{ CompileTimeHash("ApplicationStage", 16) };
    m_world.ScheduleStage(ApplicationStage, [&] {
        EXPECT_EQ(entity.FindComponent<Number>()->m_value, 7);
    });
    m_world.SchedulePhase(Phases::PostUpdate);
    ASSERT_TRUE(m_world.ExecuteSchedule());
    EXPECT_EQ(entity.FindComponent<Number>()->m_value, 26);
    ASSERT_TRUE(m_world.EndUpdate());
}


TEST_F(WorldFixture, FrameworkCyclesAreRejectedBeforeAnyExecution)
{
    Spawn();
    m_world.BeginUpdate();
    EntityUpdateContext context{ m_world, nullptr, m_world.GetEpoch() };
    auto a = Query<Number>::Create().Traverse(context, Phases::Update, [](Number&) {
        ADD_FAILURE();
    });
    auto b = Query<Number>::Create().SetPrerequisites({ a }).Traverse(context, Phases::Update, [](Number&) {
        ADD_FAILURE();
    });
    ASSERT_TRUE(m_world.AddPrerequisite(*a, b));
    m_world.SchedulePhase(Phases::Update);
    EXPECT_DEATH(m_world.ValidateSchedule(), "");
    ExpectIncompleteUpdate(m_world);
}


TEST_F(WorldFixture, OptionalParentTermsExecuteOnRoots)
{
    Spawn();
    m_world.BeginUpdate();
    EntityUpdateContext context{ m_world, nullptr, m_world.GetEpoch() };
    CascadeQuery<const Number, Parent<const Number*>, Extra*>::Create().Traverse(
        context,
        Phases::Update,
        [](Entity&, const Number&, const Number* parent, Extra*) {
            EXPECT_EQ(parent, nullptr);
        });
    m_world.SchedulePhase(Phases::Update);
    EXPECT_TRUE(m_world.ExecuteSchedule());
    EXPECT_TRUE(m_world.EndUpdate());
}


TEST_F(WorldFixture, ExistingDestructionInACreationListDoesNotWaitForNextFrame)
{
    Entity& entity = Spawn();
    EntityID id = entity.GetID();
    m_world.BeginUpdate();
    EntityCommandList commands(m_world);
    auto token = commands.CreateEntity(m_registry, {}, kOther);
    commands.AddComponent(token, Number{ 3 });
    commands.Destroy(id);
    m_world.Submit(std::move(commands));
    ASSERT_TRUE(m_world.Commit());
    EXPECT_EQ(m_world.Find(id), nullptr);
    EXPECT_EQ(m_world.Find(kOther, false), nullptr);
    ASSERT_TRUE(m_world.EndUpdate());
    m_world.BeginUpdate();
    EXPECT_NE(m_world.Find(kOther), nullptr);
    ASSERT_TRUE(m_world.EndUpdate());
}


TEST_F(WorldFixture, ReadyAdditionsPublishWhileOtherAdditionsAreStillLoading)
{
    Entity& entity = Spawn();
    Hook::s_pending = 1;
    EntityCommandList commands(m_world);
    commands.AddComponent(entity.GetID(), Hook{ 1 });
    commands.AddComponent(entity.GetID(), Extra{ 23 });
    m_world.Submit(std::move(commands));
    ASSERT_TRUE(m_world.CommitBootstrap());
    EXPECT_EQ((Count<const Extra, const Hook*>([](const Extra& extra, const Hook* hook) {
                  EXPECT_EQ(extra.m_value, 23);
                  EXPECT_EQ(hook, nullptr);
              })),
              1);
    Hook::s_pending = 0;
}


TEST_F(WorldFixture, FailedNewChildDoesNotDeactivateItsActiveParent)
{
    Entity& entity = Spawn();
    Hook::s_failStage = 3;
    EntityCommandList commands(m_world);
    auto child = commands.CreateEntity(m_registry, {}, kChild);
    commands.AddComponent(child, Hook{ 2 });
    commands.SetParent(child, entity.GetID());
    m_world.Submit(std::move(commands));
    ASSERT_TRUE(m_world.CommitBootstrap());
    EXPECT_TRUE(entity.IsActive());
    EXPECT_FALSE(entity.HasFailed());
    EXPECT_TRUE(m_world.Find(kChild, false)->HasFailed());
    EXPECT_EQ(Hook::s_trace, (festd::vector<int32_t>{ 21, 22, 23, 24, 25, 26 }));
    Hook::s_failStage = 0;
}


TEST_F(WorldFixture, RegistryUnloadCancelsQueuedCreationsAndPendingReplacements)
{
    m_assets.m_status = LifecycleResult::kSucceeded;
    EntityCommandList commands(m_world);
    auto token = commands.CreateEntity(m_registry, {}, kRoot);
    commands.AddComponent(token, CustomLoad{ kAsset });
    m_world.Submit(std::move(commands));
    ASSERT_TRUE(m_world.CommitBootstrap());
    m_assets.m_status = LifecycleResult::kPending;
    EntityCommandList replace(m_world);
    replace.ReplaceComponent(m_world.Find(kRoot)->GetID(), CustomLoad{ kOther });
    m_world.Submit(std::move(replace));
    ASSERT_TRUE(m_world.CommitBootstrap());
    EXPECT_EQ(m_assets.m_acquires, 2);
    m_world.BeginUpdate();
    EntityCommandList pending(m_world);
    pending.CreateEntity(m_registry, {}, kChild);
    m_world.Submit(std::move(pending));
    EntityCommandList unload(m_world);
    unload.UnloadRegistry(m_registry);
    m_world.Submit(std::move(unload));
    ASSERT_TRUE(m_world.Commit());
    EXPECT_EQ(m_world.GetEntityCount(), 0);
    EXPECT_EQ(m_assets.m_releases, 2);
    ASSERT_TRUE(m_world.EndUpdate());
    m_world.BeginUpdate();
    EXPECT_EQ(m_world.Find(kChild, false), nullptr);
    ASSERT_TRUE(m_world.EndUpdate());
}


TEST_F(WorldFixture, MigrationDoesNotRepeatLifecycleHooksForRetainedComponents)
{
    EntityCommandList commands(m_world);
    auto token = commands.CreateEntity(m_registry, {}, kRoot);
    commands.AddComponent(token, Hook{ 1 });
    m_world.Submit(std::move(commands));
    ASSERT_TRUE(m_world.CommitBootstrap());
    Hook::s_trace.clear();
    EntityCommandList migrate(m_world);
    migrate.AddComponent(m_world.Find(kRoot)->GetID(), Number{ 17 });
    m_world.Submit(std::move(migrate));
    ASSERT_TRUE(m_world.CommitBootstrap());
    EXPECT_TRUE(Hook::s_trace.empty());
    EXPECT_EQ(m_world.Find(kRoot)->FindComponent<Hook>()->m_label, 1);
}


TEST_F(WorldFixture, ComponentsCanBeAddedWithoutInitializationPrerequisites)
{
    EntityCommandList commands(m_world);
    auto token = commands.CreateEntity(m_registry, {}, kRoot);
    commands.AddComponent(token, Extra{ 2 });
    m_world.Submit(std::move(commands));
    ASSERT_TRUE(m_world.CommitBootstrap());

    Entity* entity = m_world.Find(kRoot);
    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->FindComponent<const Extra>()->m_value, 2);

    EntityCommandList add(m_world);
    add.AddComponent(entity->GetID(), Number{ 1 });
    m_world.Submit(std::move(add));
    ASSERT_TRUE(m_world.CommitBootstrap());
    EXPECT_EQ(entity->FindComponent<const Number>()->m_value, 1);
    EXPECT_EQ(entity->FindComponent<const Extra>()->m_value, 2);
}


TEST_F(WorldFixture, MultipleChunksCompactAndReleaseMoveOnlyValuesAcrossRepeatedChurn)
{
    for (int round = 0; round < 3; ++round)
    {
        EntityCommandList commands(m_world);
        festd::vector<Uuid> uuids;
        for (int i = 0; i < 150; ++i)
        {
            Uuid uuid = kRoot;
            uuid.m_bytes[12] = static_cast<uint8_t>(i);
            uuid.m_bytes[13] = static_cast<uint8_t>(round);
            uuids.push_back(uuid);
            auto token = commands.CreateEntity(m_registry, {}, uuid);
            commands.AddComponent(token, MoveOnly{ i });
            commands.AddComponent(token, Aligned{});
        }
        m_world.Submit(std::move(commands));
        ASSERT_TRUE(m_world.CommitBootstrap());
        EXPECT_GT(m_world.GetChunkCount(), 1);
        EntityCommandList changes(m_world);
        for (uint32_t i = 0; i < uuids.size(); ++i)
        {
            Entity* entity = m_world.Find(uuids[i]);
            if (i % 3 == 0)
                changes.Destroy(entity->GetID());
            else
                changes.AddComponent(entity->GetID(), Extra{ static_cast<int32_t>(i) });
        }
        m_world.Submit(std::move(changes));
        ASSERT_TRUE(m_world.CommitBootstrap());
        EntityCommandList destroy(m_world);
        for (uint32_t i = 0; i < uuids.size(); ++i)
        {
            Entity* entity = m_world.Find(uuids[i]);
            if (i % 3 == 0)
            {
                EXPECT_EQ(entity, nullptr);
                continue;
            }
            ASSERT_NE(entity, nullptr);
            EXPECT_EQ(entity->FindComponent<MoveOnly>()->m_value, i);
            EXPECT_EQ(reinterpret_cast<uintptr_t>(entity->FindComponent<Aligned>()) % 256, 0);
            destroy.Destroy(entity->GetID());
        }
        m_world.Submit(std::move(destroy));
        ASSERT_TRUE(m_world.CommitBootstrap());
        EXPECT_EQ(m_world.GetEntityCount(), 0);
        EXPECT_EQ(m_world.GetChunkCount(), 0);
        EXPECT_EQ(MoveOnly::s_live, 0);
    }
}


TEST(EntityAssetsIntegration, MissingManagerDependencyFailsWithoutPublishingOrRetainingResidency)
{
    IO::ArtifactStore::SetCatalogSource(IO::Path(FE_FRAMEWORK_CORE_FIXTURES));
    IO::AssetManager::Init();
    auto shutdown = festd::defer([] {
        IO::AssetManager::Shutdown();
    });
    EntityWorld world;
    auto& registry = world.CreateRegistry();
    EntityCommandList commands(world);
    auto token = commands.CreateEntity(registry, {}, kRoot);
    commands.AddComponent(token, CustomLoad{ kAsset });
    world.Submit(std::move(commands));
    ASSERT_TRUE(world.CommitBootstrap());
    auto probe = IO::AssetManager::LoadAsset(kAsset);
    probe.WaitForDiscovery();
    IO::AssetManager::Tick();
    ASSERT_TRUE(world.CommitBootstrap());
    auto* entity = world.Find(kRoot, false);
    ASSERT_NE(entity, nullptr);
    EXPECT_TRUE(entity->HasFailed());
    EXPECT_FALSE(entity->IsActive());
    auto* slot = IO::AssetManager::FindAssetSlot(kAsset);
    ASSERT_NE(slot, nullptr);
    EXPECT_EQ(slot->m_strongRefCount.load(), 0);
}


TEST_F(WorldFixture, DestructionAndDetachRespectRecordingOrder)
{
    for (int scenario = 0; scenario < 2; ++scenario)
    {
        EntityCommandList commands(m_world);
        auto root = commands.CreateEntity(m_registry, {}, kRoot);
        auto child = commands.CreateEntity(m_registry, {}, kChild);
        commands.SetParent(child, root);
        m_world.Submit(std::move(commands));
        ASSERT_TRUE(m_world.CommitBootstrap());
        EntityID rootID = m_world.Find(kRoot)->GetID();
        EntityID childID = m_world.Find(kChild)->GetID();
        EntityCommandList destroy(m_world);
        if (scenario == 0)
            destroy.SetParent(childID);
        destroy.Destroy(rootID);
        if (scenario == 1)
            destroy.SetParent(childID);
        m_world.Submit(std::move(destroy));
        ASSERT_TRUE(m_world.CommitBootstrap());
        if (scenario == 0)
        {
            ASSERT_NE(m_world.Find(childID), nullptr);
            EntityCommandList cleanup(m_world);
            cleanup.Destroy(childID);
            m_world.Submit(std::move(cleanup));
            ASSERT_TRUE(m_world.CommitBootstrap());
        }
        else
            EXPECT_EQ(m_world.Find(childID), nullptr);
    }
}


TEST_F(WorldFixture, OverlappingSubtreeCommandsFromIndependentListsAreRejected)
{
    EntityCommandList commands(m_world);
    auto root = commands.CreateEntity(m_registry, {}, kRoot);
    auto child = commands.CreateEntity(m_registry, {}, kChild);
    commands.SetParent(child, root);
    m_world.Submit(std::move(commands));
    ASSERT_TRUE(m_world.CommitBootstrap());
    Entity* r = m_world.Find(kRoot);
    Entity* c = m_world.Find(kChild);
    EntityCommandList destroy(m_world), detach(m_world);
    destroy.Destroy(r->GetID());
    detach.SetParent(c->GetID());
    m_world.Submit(std::move(destroy));
    m_world.Submit(std::move(detach));
    EXPECT_FALSE(m_world.CommitBootstrap());
    EXPECT_EQ(m_world.Find(kRoot), r);
    EXPECT_EQ(c->GetParent(), r);
}


TEST_F(WorldFixture, SequentialAndParallelQueriesExecute)
{
    Spawn();
    m_world.BeginUpdate();
    EntityUpdateContext context{ m_world, nullptr, m_world.GetEpoch() };
    Query<Number>::Create().SetExecutionPolicy(ExecutionPolicy::kParallel).Traverse(context, Phases::Update, [](Number& number) {
        ++number.m_value;
    });
    m_world.SchedulePhase(Phases::Update);
    EXPECT_TRUE(m_world.ExecuteSchedule());
    EXPECT_TRUE(m_world.EndUpdate());
    m_world.BeginUpdate();
    EntityUpdateContext next{ m_world, nullptr, m_world.GetEpoch() };
    auto group = Query<Number>::Create()
                     .SetExecutionPolicy(ExecutionPolicy::kSequential)
                     .Traverse(next, Phases::Update, [](Number& number) {
                         ++number.m_value;
                     });
    m_world.SchedulePhase(Phases::Update);
    EXPECT_TRUE(m_world.ExecuteSchedule());
    EXPECT_TRUE(group->IsSignaled());
    EXPECT_TRUE(m_world.EndUpdate());
}


TEST_F(WorldFixture, MainThreadChangedTraversalVisitsChunksWithoutSubdivision)
{
    EntityCommandList commands(m_world);
    for (Uuid uuid : { kRoot, kOther })
    {
        auto token = commands.CreateEntity(m_registry, {}, uuid);
        commands.AddComponent<Huge>(token);
        commands.AddComponent(token, Number{ 3 });
    }
    m_world.Submit(std::move(commands));
    ASSERT_TRUE(m_world.CommitBootstrap());
    ASSERT_EQ(m_world.GetChunkCount(), 2);

    ChangeCursor cursor;
    for (uint32_t epoch = 0; epoch < 2; ++epoch)
    {
        m_world.BeginUpdate();
        EntityUpdateContext context{ m_world, nullptr, m_world.GetEpoch() };
        uint32_t callbacks = 0;
        auto completion = Query<const Huge, Number>::Create()
                              .SetChangeCursor(cursor)
                              .SetExecutionPolicy(ExecutionPolicy::kMainThread)
                              .Traverse(context, Phases::Update, [&](Entity& entity, const Huge&, Number& number) {
                                  EXPECT_TRUE(Threading::IsMainThread());
                                  EXPECT_EQ(entity.FindComponent<Number>(), &number);
                                  ++number.m_value;
                                  ++callbacks;
                              });
        EXPECT_FALSE(completion->IsSignaled());
        ASSERT_TRUE(m_world.SchedulePhase(Phases::Update));
        ASSERT_TRUE(m_world.ExecuteSchedule());
        EXPECT_TRUE(completion->IsSignaled());
        EXPECT_EQ(callbacks, epoch == 0 ? 2 : 0);
        EXPECT_EQ(m_world.GetScheduleDiagnostics().m_jobs, 1);
        ASSERT_TRUE(m_world.EndUpdate());
    }
}


TEST_F(WorldFixture, MainThreadTraversalHonorsReverseWorkerPrerequisite)
{
    Spawn();
    m_world.BeginUpdate();
    EntityUpdateContext context{ m_world, nullptr, m_world.GetEpoch() };
    auto read = Query<const Number>::Create()
                    .SetExecutionPolicy(ExecutionPolicy::kMainThread)
                    .Traverse(context, Phases::Update, [](const Number& number) {
                        EXPECT_TRUE(Threading::IsMainThread());
                        EXPECT_EQ(number.m_value, 42);
                    });
    auto write = Query<Number>::Create()
                     .SetExecutionPolicy(ExecutionPolicy::kParallel)
                     .Traverse(context, Phases::Update, [](Number& number) {
                         number.m_value = 42;
                     });
    ASSERT_TRUE(m_world.AddPrerequisite(*read, write));
    ASSERT_TRUE(m_world.SchedulePhase(Phases::Update));
    ASSERT_TRUE(m_world.ExecuteSchedule());
    EXPECT_TRUE(read->IsSignaled());
    ASSERT_TRUE(m_world.EndUpdate());
}


TEST_F(WorldFixture, ReadOnlyTraversalsOverlapThroughFiberBarriers)
{
    Spawn();
    m_world.BeginUpdate();
    EntityUpdateContext context{ m_world, nullptr, m_world.GetEpoch() };
    const Rc<WaitGroup> entered = WaitGroup::Create(2);
    std::atomic<uint32_t> count = 0;
    auto callback = [&](const Number&) {
        count.fetch_add(1);
        entered->Signal();
        entered->Wait();
    };
    Query<const Number>::Create().Traverse(context, Phases::Update, callback);
    Query<const Number>::Create().Traverse(context, Phases::Update, callback);
    ASSERT_TRUE(m_world.SchedulePhase(Phases::Update));
    ASSERT_TRUE(m_world.ExecuteSchedule());
    EXPECT_EQ(count.load(), 2);
    EXPECT_EQ(m_world.GetScheduleDiagnostics().m_conflictEdges, 0);
    ASSERT_TRUE(m_world.EndUpdate());
}


TEST_F(WorldFixture, ExplicitReversePrerequisiteOrientsConflictingWork)
{
    Spawn();
    m_world.BeginUpdate();
    EntityUpdateContext context{ m_world, nullptr, m_world.GetEpoch() };
    auto read = Query<const Number>::Create().Traverse(context, Phases::Update, [](const Number& value) {
        EXPECT_EQ(value.m_value, 42);
    });
    auto write = Query<Number>::Create().Traverse(context, Phases::Update, [](Number& value) {
        value.m_value = 42;
    });
    ASSERT_TRUE(m_world.AddPrerequisite(*read, write));
    ASSERT_TRUE(m_world.SchedulePhase(Phases::Update));
    ASSERT_TRUE(m_world.ExecuteSchedule());
    EXPECT_GT(m_world.GetScheduleDiagnostics().m_conflictEdges, 0);
    ASSERT_FALSE(m_world.GetScheduleConflicts().empty());
    EXPECT_EQ(m_world.GetScheduleConflicts()[0].m_component, Rtti::GetTypeID<Number>());
    ASSERT_TRUE(m_world.EndUpdate());
}


TEST_F(WorldFixture, DependencyCompilationPreservesConflictOrdering)
{
    Spawn();
    for (bool reverse : { false, true })
    {
        m_world.BeginUpdate();
        EntityUpdateContext context{ m_world, nullptr, m_world.GetEpoch() };
        festd::inline_vector<uint32_t> order;
        auto first = Query<Number>::Create().Traverse(context, Phases::Update, [&](Number&) {
            order.push_back(0);
        });
        auto second = Query<Number>::Create().Traverse(context, Phases::Update, [&](Number&) {
            order.push_back(1);
        });
        Query<Number>::Create().Traverse(context, Phases::Update, [&](Number&) {
            order.push_back(2);
        });
        ASSERT_TRUE(m_world.AddPrerequisite(reverse ? *first : *second, reverse ? second : first));
        ASSERT_TRUE(m_world.SchedulePhase(Phases::Update));
        ASSERT_TRUE(m_world.ExecuteSchedule());
        ASSERT_EQ(order.size(), 3);
        EXPECT_EQ(order[0], reverse ? 1 : 0);
        EXPECT_EQ(order[1], reverse ? 2 : 1);
        EXPECT_EQ(order[2], reverse ? 0 : 2);
        ASSERT_TRUE(m_world.EndUpdate());
    }
}


TEST_F(WorldFixture, ParallelChunksOverlapAndKeepEntityLookupAccessOnSuspendedFibers)
{
    EntityCommandList commands(m_world);
    for (Uuid uuid : { kRoot, kOther })
    {
        auto token = commands.CreateEntity(m_registry, {}, uuid);
        commands.AddComponent<Huge>(token);
        commands.AddComponent(token, Number{ 3 });
    }
    m_world.Submit(std::move(commands));
    ASSERT_TRUE(m_world.CommitBootstrap());
    ASSERT_EQ(m_world.GetChunkCount(), 2);
    m_world.BeginUpdate();
    EntityUpdateContext context{ m_world, nullptr, m_world.GetEpoch() };
    const Rc<WaitGroup> entered = WaitGroup::Create(2);
    Query<const Huge, Number>::Create()
        .SetExecutionPolicy(ExecutionPolicy::kParallel)
        .Traverse(context, Phases::Update, [&](Entity& entity, const Huge&, Number& number) {
            entered->Signal();
            entered->Wait();
            EXPECT_EQ(entity.FindComponent<Number>(), &number);
            ++number.m_value;
        });
    ASSERT_TRUE(m_world.SchedulePhase(Phases::Update));
    ASSERT_TRUE(m_world.ExecuteSchedule());
    EXPECT_EQ(m_world.GetScheduleDiagnostics().m_callbacks, 2);
    EXPECT_GE(m_world.GetScheduleDiagnostics().m_jobs, 3);
    ASSERT_TRUE(m_world.EndUpdate());
}


TEST_F(WorldFixture, CascadeTraversesFilteredAncestorsAndUsesImmediateParents)
{
    EntityCommandList commands(m_world);
    auto root = commands.CreateEntity(m_registry, {}, kRoot);
    auto child = commands.CreateEntity(m_registry, {}, kChild);
    auto leaf = commands.CreateEntity(m_registry, {}, kOther);
    commands.AddComponent(root, Number{ 10 });
    commands.AddComponent(child, Extra{ 1 });
    commands.AddComponent(leaf, Number{ 3 });
    commands.AddComponent(leaf, Extra{ 1 });
    commands.SetParent(child, root);
    commands.SetParent(leaf, child);
    m_world.Submit(std::move(commands));
    ASSERT_TRUE(m_world.CommitBootstrap());
    m_world.BeginUpdate();
    EntityUpdateContext context{ m_world, nullptr, m_world.GetEpoch() };
    uint32_t required = 0;
    uint32_t optional = 0;
    CascadeQuery<Extra, Parent<const Number>>::Create().Traverse(context,
                                                                 Phases::Update,
                                                                 [&](Extra& extra, const Number& parent) {
                                                                     ++required;
                                                                     extra.m_value += parent.m_value;
                                                                 });
    CascadeQuery<const Extra, Parent<const Number*>>::Create().Traverse(context,
                                                                        Phases::Update,
                                                                        [&](Entity& entity, const Extra&, const Number* parent) {
                                                                            ++optional;
                                                                            EXPECT_EQ(parent != nullptr,
                                                                                      entity.GetUuid() == kChild);
                                                                        });
    ASSERT_TRUE(m_world.SchedulePhase(Phases::Update));
    ASSERT_TRUE(m_world.ExecuteSchedule());
    EXPECT_EQ(required, 1);
    EXPECT_EQ(optional, 2);
    ASSERT_TRUE(m_world.EndUpdate());
}


TEST_F(WorldFixture, CascadeParentCompletesBeforeChildForAllPolicies)
{
    EntityCommandList commands(m_world);
    auto root = commands.CreateEntity(m_registry, {}, kRoot);
    auto child = commands.CreateEntity(m_registry, {}, kChild);
    auto leaf = commands.CreateEntity(m_registry, {}, kOther);
    for (auto token : { root, child, leaf })
        commands.AddComponent(token, Number{ 1 });
    commands.SetParent(child, root);
    commands.SetParent(leaf, child);
    m_world.Submit(std::move(commands));
    ASSERT_TRUE(m_world.CommitBootstrap());
    for (auto policy : { ExecutionPolicy::kSequential, ExecutionPolicy::kParallel, ExecutionPolicy::kMainThread })
    {
        m_world.BeginUpdate();
        EntityUpdateContext context{ m_world, nullptr, m_world.GetEpoch() };
        CascadeQuery<Number, Parent<const Number*>>::Create().SetExecutionPolicy(policy).Traverse(
            context,
            Phases::Update,
            [policy](Number& number, const Number* parent) {
                if (policy == ExecutionPolicy::kMainThread)
                    EXPECT_TRUE(Threading::IsMainThread());

                number.m_value = parent ? parent->m_value + 1 : 1;
            });
        ASSERT_TRUE(m_world.SchedulePhase(Phases::Update));
        ASSERT_TRUE(m_world.ExecuteSchedule());
        EXPECT_EQ(m_world.Find(kOther)->FindComponent<Number>()->m_value, 3);
        ASSERT_TRUE(m_world.EndUpdate());
    }
}


TEST_F(WorldFixture, IndependentTreeBatchesOverlap)
{
    EntityCommandList commands(m_world);
    for (uint32_t i = 0; i < 33; ++i)
    {
        auto token = commands.CreateEntity(m_registry);
        commands.AddComponent(token, Number{ static_cast<int32_t>(i) });
    }
    m_world.Submit(std::move(commands));
    ASSERT_TRUE(m_world.CommitBootstrap());
    m_world.BeginUpdate();
    EntityUpdateContext context{ m_world, nullptr, m_world.GetEpoch() };
    const Rc<WaitGroup> entered = WaitGroup::Create(2);
    CascadeQuery<Number>::Create()
        .SetExecutionPolicy(ExecutionPolicy::kParallel)
        .Traverse(context, Phases::Update, [&](Number& number) {
            if (number.m_value == 0 || number.m_value == 16)
            {
                entered->Signal();
                entered->Wait();
            }
            ++number.m_value;
        });
    ASSERT_TRUE(m_world.SchedulePhase(Phases::Update));
    ASSERT_TRUE(m_world.ExecuteSchedule());
    EXPECT_EQ(m_world.GetScheduleDiagnostics().m_treeBatches, 3);
    ASSERT_TRUE(m_world.EndUpdate());
}


TEST_F(WorldFixture, SameSystemSequentialWorkGetsExclusionAndParallelWorkCanOverlap)
{
    Entity& entity = Spawn();
    EntityCommandList commands(m_world);
    commands.AddComponent<Extra>(entity.GetID());
    m_world.Submit(std::move(commands));
    ASSERT_TRUE(m_world.CommitBootstrap());
    struct System final : WorldSystem
    {
        ExecutionPolicy m_policy = ExecutionPolicy::kSequential;
        Rc<WaitGroup> m_entered;
        std::atomic<uint32_t> m_inside = 0;
        void Update(EntityUpdateContext& context) override
        {
            auto callback = [&] {
                const uint32_t previous = m_inside.fetch_add(1);
                if (m_policy != ExecutionPolicy::kParallel)
                    EXPECT_EQ(previous, 0);
                else
                {
                    m_entered->Signal();
                    m_entered->Wait();
                }
                m_inside.fetch_sub(1);
            };
            Query<const Number>::Create().SetExecutionPolicy(m_policy).Traverse(context,
                                                                                Phases::Update,
                                                                                [callback](const Number&) {
                                                                                    callback();
                                                                                });
            Query<const Extra>::Create().SetExecutionPolicy(m_policy).Traverse(context, Phases::Update, [callback](const Extra&) {
                callback();
            });
        }
    } system;
    m_world.AddSystem(system);
    for (auto policy : { ExecutionPolicy::kSequential, ExecutionPolicy::kParallel, ExecutionPolicy::kMainThread })
    {
        system.m_policy = policy;
        system.m_entered = WaitGroup::Create(2);
        m_world.BeginUpdate();
        ASSERT_TRUE(m_world.SchedulePhase(Phases::Update));
        ASSERT_TRUE(m_world.ExecuteSchedule());
        EXPECT_EQ(m_world.GetScheduleDiagnostics().m_systemEdges, policy != ExecutionPolicy::kParallel ? 1 : 0);
        ASSERT_TRUE(m_world.EndUpdate());
    }
    m_world.RemoveSystem(system);
}


TEST_F(WorldFixture, ChangedConsumersAreIndependentAndAdvanceOnlyAfterCompletion)
{
    Entity& entity = Spawn();
    ChangeCursor first, second;
    auto consume = [&](ChangeCursor& cursor) {
        m_world.BeginUpdate();
        EntityUpdateContext context{ m_world, nullptr, m_world.GetEpoch() };
        uint32_t count = 0;
        const uint64_t oldVersion = cursor.m_version;
        Query<const Number>::Create().SetChangeCursor(cursor).Traverse(context, Phases::Update, [&](const Number&) {
            ++count;
        });
        EXPECT_EQ(cursor.m_version, oldVersion);
        EXPECT_TRUE(m_world.SchedulePhase(Phases::Update));
        EXPECT_TRUE(m_world.ExecuteSchedule());
        EXPECT_TRUE(m_world.EndUpdate());
        return count;
    };
    EXPECT_EQ(consume(first), 1);
    EXPECT_EQ(consume(first), 0);
    EXPECT_EQ(consume(second), 1);
    EntityCommandList commands(m_world);
    commands.AddComponent<Extra>(entity.GetID());
    m_world.Submit(std::move(commands));
    EXPECT_EQ(consume(first), 1);
    EXPECT_EQ(consume(second), 1);
}


TEST_F(WorldFixture, ConflictingCallbacksCannotOverlapWhileAnIndependentProbeRuns)
{
    Spawn();
    const Rc<WaitGroup> entered = WaitGroup::Create();
    const Rc<WaitGroup> release = WaitGroup::Create();
    const Rc<WaitGroup> probeDone = WaitGroup::Create();
    std::atomic<bool> readerEntered = false;
    m_world.BeginUpdate();
    EntityUpdateContext context{ m_world, nullptr, m_world.GetEpoch() };
    Query<Number>::Create().Traverse(context, Phases::Update, [&](Number& number) {
        entered->Signal();
        release->Wait();
        number.m_value = 99;
    });
    Query<const Number>::Create().Traverse(context, Phases::Update, [&](const Number& number) {
        readerEntered.store(true);
        EXPECT_EQ(number.m_value, 99);
    });
    Jobs::DispatchMainThread(
        { entered },
        [&] {
            EXPECT_FALSE(readerEntered.load());
            release->Signal();
        },
        probeDone.Get());
    ASSERT_TRUE(m_world.SchedulePhase(Phases::Update));
    ASSERT_TRUE(m_world.ExecuteSchedule());
    probeDone->Wait();
    EXPECT_TRUE(readerEntered.load());
    ASSERT_TRUE(m_world.EndUpdate());
}


TEST_F(WorldFixture, WritesPublishChangesForTwoIndependentConsumers)
{
    Spawn();
    ChangeCursor first, second;
    auto tick = [&](bool write) {
        m_world.BeginUpdate();
        EntityUpdateContext context{ m_world, nullptr, m_world.GetEpoch() };
        if (write)
        {
            Query<Number>::Create().Traverse(context, Phases::PreUpdate, [](Number& number) {
                ++number.m_value;
            });
            EXPECT_TRUE(m_world.SchedulePhase(Phases::PreUpdate));
        }
        uint32_t count = 0;
        Query<const Number>::Create().SetChangeCursor(first).Traverse(context, Phases::Update, [&](const Number&) {
            ++count;
        });
        Query<const Number>::Create().SetChangeCursor(second).Traverse(context, Phases::Update, [&](const Number&) {
            ++count;
        });
        EXPECT_TRUE(m_world.SchedulePhase(Phases::Update));
        EXPECT_TRUE(m_world.ExecuteSchedule());
        EXPECT_TRUE(m_world.EndUpdate());
        return count;
    };
    EXPECT_EQ(tick(false), 2);
    EXPECT_EQ(tick(false), 0);
    EXPECT_EQ(tick(true), 2);
    EXPECT_EQ(tick(false), 0);
}


TEST_F(WorldFixture, ConditionalWritesPublishMixedChunksForIndependentParallelConsumers)
{
    EntityCommandList commands(m_world);
    for (uint32_t i = 0; i < 1024; ++i)
    {
        auto entity = commands.CreateEntity(m_registry);
        commands.AddComponent(entity, Number{ i < 512 ? 1 : 0 });
    }
    m_world.Submit(std::move(commands));
    ASSERT_TRUE(m_world.CommitBootstrap());
    ChangeCursor first, second;
    auto tick = [&](const bool write, const bool cascade) {
        m_world.BeginUpdate();
        EntityUpdateContext context{ m_world, nullptr, m_world.GetEpoch() };
        if (write)
        {
            const auto update = [](Number& number) {
                if (number.m_value == 0)
                    return QueryWriteResult::kUnchanged;
                ++number.m_value;
                return QueryWriteResult::kChanged;
            };
            if (cascade)
                CascadeQuery<Number>::Create()
                    .SetExecutionPolicy(ExecutionPolicy::kParallel)
                    .Traverse(context, Phases::PreUpdate, update);
            else
                Query<Number>::Create()
                    .SetExecutionPolicy(ExecutionPolicy::kParallel)
                    .Traverse(context, Phases::PreUpdate, update);
            EXPECT_TRUE(m_world.SchedulePhase(Phases::PreUpdate));
        }
        std::atomic<uint32_t> firstCount = 0, secondCount = 0;
        Query<const Number>::Create()
            .SetChangeCursor(first)
            .SetExecutionPolicy(ExecutionPolicy::kParallel)
            .Traverse(context, Phases::Update, [&](const Number&) {
                ++firstCount;
            });
        Query<const Number>::Create()
            .SetChangeCursor(second)
            .SetExecutionPolicy(ExecutionPolicy::kParallel)
            .Traverse(context, Phases::Update, [&](const Number&) {
                ++secondCount;
            });
        EXPECT_TRUE(m_world.SchedulePhase(Phases::Update));
        EXPECT_TRUE(m_world.ExecuteSchedule());
        EXPECT_TRUE(m_world.EndUpdate());
        EXPECT_EQ(firstCount.load(), secondCount.load());
        return firstCount.load();
    };
    EXPECT_EQ(tick(false, false), 1024);
    EXPECT_EQ(tick(false, false), 0);
    EXPECT_GE(tick(true, false), 512);
    EXPECT_GT(m_world.GetScheduleDiagnostics().m_chunks, 1);
    EXPECT_EQ(tick(false, false), 0);
    EXPECT_GE(tick(true, true), 512);
    EXPECT_EQ(tick(false, false), 0);
}


TEST_F(WorldFixture, SharedChangeCursorAndAliasingParallelParentTermsAreRejected)
{
    Spawn();
    ChangeCursor changes;
    m_world.BeginUpdate();
    EntityUpdateContext context{ m_world, nullptr, m_world.GetEpoch() };
    Query<const Number>::Create().SetChangeCursor(changes).Traverse(context, Phases::Update, [](const Number&) {
        ADD_FAILURE();
    });
    Query<const Number>::Create().SetChangeCursor(changes).Traverse(context, Phases::Update, [](const Number&) {
        ADD_FAILURE();
    });
    ASSERT_TRUE(m_world.SchedulePhase(Phases::Update));
    EXPECT_DEATH(m_world.ValidateSchedule(), "");
    ExpectIncompleteUpdate(m_world);
}


TEST_F(WorldFixture, AliasingParallelParentTermsAssert)
{
    Spawn();
    m_world.BeginUpdate();
    EntityUpdateContext next{ m_world, nullptr, m_world.GetEpoch() };
    Query<Number, Parent<const Number*>>::Create()
        .SetExecutionPolicy(ExecutionPolicy::kParallel)
        .Traverse(next, Phases::Update, [](Number&, const Number*) {
            ADD_FAILURE();
        });
    ASSERT_TRUE(m_world.SchedulePhase(Phases::Update));
    if (Cli::Check("--ecs-validation"))
        EXPECT_DEATH(m_world.ExecuteSchedule(), "");
    else
        EXPECT_DEATH(m_world.ValidateSchedule(), "");

    ExpectIncompleteUpdate(m_world);
}


TEST_F(WorldFixture, MixedPublicationPreservesDetachAndDestroyRecordingOrder)
{
    for (uint32_t scenario = 0; scenario < 2; ++scenario)
    {
        EntityCommandList setup(m_world);
        auto parent = setup.CreateEntity(m_registry, {}, kRoot);
        auto child = setup.CreateEntity(m_registry, {}, kChild);
        setup.SetParent(child, parent);
        m_world.Submit(std::move(setup));
        if (scenario == 0)
            ASSERT_TRUE(m_world.CommitBootstrap());
        else
        {
            m_world.BeginUpdate();
            ASSERT_TRUE(m_world.EndUpdate());
        }

        const EntityID parentId = m_world.Find(kRoot)->GetID();
        const EntityID childId = m_world.Find(kChild)->GetID();
        m_world.BeginUpdate();
        EntityCommandList edits(m_world);
        edits.CreateEntity(m_registry, {}, kOther);
        edits.AddComponent(parentId, Extra{ 42 });
        if (scenario == 0)
            edits.SetParent(childId);
        edits.Destroy(parentId);
        if (scenario == 1)
            edits.SetParent(childId);
        m_world.Submit(std::move(edits));
        ASSERT_TRUE(m_world.Commit());
        ASSERT_TRUE(m_world.EndUpdate());

        m_world.BeginUpdate();
        EXPECT_EQ(m_world.Find(parentId), nullptr);
        EXPECT_NE(m_world.Find(kOther), nullptr);
        EXPECT_EQ(m_world.Find(childId) != nullptr, scenario == 0);
        if (scenario == 0)
        {
            Entity* survivor = m_world.Find(childId);
            ASSERT_NE(survivor, nullptr);
            EXPECT_EQ(survivor->GetParent(), nullptr);
        }
        ASSERT_TRUE(m_world.EndUpdate());

        EntityCommandList cleanup(m_world);
        cleanup.Destroy(m_world.Find(kOther)->GetID());
        if (scenario == 0)
            cleanup.Destroy(childId);
        m_world.Submit(std::move(cleanup));
        ASSERT_TRUE(m_world.Commit());
    }
}


TEST_F(WorldFixture, DeliberatelyInactiveChildDoesNotBlockParentActivation)
{
    EntityCommandList setup(m_world);
    auto parent = setup.CreateEntity(m_registry, {}, kRoot);
    auto child = setup.CreateEntity(m_registry, {}, kChild);
    setup.AddComponent(parent, Hook{ 1 });
    setup.AddComponent(child, Hook{ 2 });
    setup.SetParent(child, parent);
    setup.SetActive(child, false);
    m_world.Submit(std::move(setup));
    ASSERT_TRUE(m_world.CommitBootstrap());
    Entity* root = m_world.Find(kRoot, false);
    Entity* inactiveChild = m_world.Find(kChild, false);
    ASSERT_NE(root, nullptr);
    ASSERT_NE(inactiveChild, nullptr);
    EXPECT_TRUE(root->IsActive());
    EXPECT_FALSE(inactiveChild->IsActive());
    EXPECT_EQ(Hook::s_trace, (festd::vector<int32_t>{ 11, 12, 13 }));

    EntityCommandList deactivate(m_world);
    deactivate.SetActive(root->GetID(), false);
    m_world.Submit(std::move(deactivate));
    ASSERT_TRUE(m_world.CommitBootstrap());
    EntityCommandList reactivate(m_world);
    reactivate.SetActive(root->GetID(), true);
    m_world.Submit(std::move(reactivate));
    ASSERT_TRUE(m_world.CommitBootstrap());
    EXPECT_TRUE(root->IsActive());
    EXPECT_FALSE(inactiveChild->IsActive());
}


TEST_F(WorldFixture, CommittedReplacementSurvivesDeactivation)
{
    Entity& entity = Spawn();
    EntityCommandList edits(m_world);
    edits.ReplaceComponent(entity.GetID(), Number{ 42 });
    edits.SetActive(entity.GetID(), false);
    m_world.Submit(std::move(edits));
    ASSERT_TRUE(m_world.CommitBootstrap());
    EXPECT_FALSE(entity.IsActive());
    EXPECT_EQ(entity.FindComponent<Number>()->m_value, 42);

    EntityCommandList reactivate(m_world);
    reactivate.SetActive(entity.GetID(), true);
    m_world.Submit(std::move(reactivate));
    ASSERT_TRUE(m_world.CommitBootstrap());
    EXPECT_TRUE(entity.IsActive());
    EXPECT_EQ(entity.FindComponent<Number>()->m_value, 42);
}


TEST_F(WorldFixture, DeactivationCancelsCandidateLoadingButRetainsItsAuthoredValue)
{
    m_assets.m_status = LifecycleResult::kSucceeded;
    EntityCommandList setup(m_world);
    auto entity = setup.CreateEntity(m_registry, {}, kRoot);
    setup.AddComponent(entity, AssetComponent{ IO::Link<Number>(kAsset), {} });
    m_world.Submit(std::move(setup));
    ASSERT_TRUE(m_world.CommitBootstrap());
    const EntityID id = m_world.Find(kRoot)->GetID();

    m_assets.m_status = LifecycleResult::kPending;
    EntityCommandList replacement(m_world);
    replacement.ReplaceComponent(id, AssetComponent{ IO::Link<Number>(kOther), {} });
    m_world.Submit(std::move(replacement));
    ASSERT_TRUE(m_world.CommitBootstrap());
    EXPECT_EQ(m_assets.m_acquires, 2);
    EXPECT_EQ(m_assets.m_releases, 0);

    EntityCommandList deactivate(m_world);
    deactivate.SetActive(id, false);
    m_world.Submit(std::move(deactivate));
    ASSERT_TRUE(m_world.CommitBootstrap());
    Entity* inactive = m_world.Find(id);
    ASSERT_NE(inactive, nullptr);
    EXPECT_FALSE(inactive->IsActive());
    EXPECT_EQ(inactive->FindComponent<AssetComponent>()->m_hard.GetAssetID(), kOther);
    EXPECT_EQ(m_assets.m_releases, 2);

    m_assets.m_status = LifecycleResult::kSucceeded;
    EntityCommandList reactivate(m_world);
    reactivate.SetActive(id, true);
    m_world.Submit(std::move(reactivate));
    ASSERT_TRUE(m_world.CommitBootstrap());
    EXPECT_TRUE(inactive->IsActive());
    EXPECT_EQ(inactive->FindComponent<AssetComponent>()->m_hard.GetAssetID(), kOther);
    EXPECT_EQ(m_assets.m_acquires, 3);
}


TEST_F(WorldFixture, AdditionFiltersRequirePersistentCursor)
{
    Spawn();
    m_world.BeginUpdate();
    EntityUpdateContext context{ m_world, nullptr, m_world.GetEpoch() };
    EXPECT_DEATH(Query<const Number>::Create()
                     .SetFilter(QueryFilter::kNewEntities)
                     .Traverse(context, Phases::Update, [](const Number&) {}),
                 "");
    EXPECT_DEATH(Query<const Number>::Create()
                     .SetFilter(QueryFilter::kAddedComponents)
                     .Traverse(context, Phases::Update, [](const Number&) {}),
                 "");
    ASSERT_TRUE(m_world.EndUpdate());
}


TEST_F(WorldFixture, SingleChunkQueryRunsRowRangesOnDifferentThreads)
{
    EntityCommandList commands(m_world);
    for (uint32_t i = 0; i < 128; ++i)
    {
        const auto entity = commands.CreateEntity(m_registry);
        commands.AddComponent(entity, Number{ static_cast<int32_t>(i) });
    }

    m_world.Submit(std::move(commands));
    ASSERT_TRUE(m_world.CommitBootstrap());
    ASSERT_EQ(m_world.GetChunkCount(), 1);

    ThreadOverlap overlap;
    ChangeCursor cursor;
    m_world.BeginUpdate();
    EntityUpdateContext context{ m_world, nullptr, m_world.GetEpoch() };
    Query<Number>::Create()
        .SetChangeCursor(cursor)
        .SetExecutionPolicy(ExecutionPolicy::kParallel)
        .Traverse(context, Phases::Update, [&](Entity& entity, Number& number) {
            if (number.m_value == 0 || number.m_value == 64)
                overlap.Meet(number.m_value == 0 ? 0 : 1);

            EXPECT_EQ(entity.FindComponent<Number>(), &number);
            ++number.m_value;
            return QueryWriteResult::kChanged;
        });
    ASSERT_TRUE(m_world.SchedulePhase(Phases::Update));
    ASSERT_TRUE(m_world.ExecuteSchedule());
    EXPECT_EQ(m_world.GetScheduleDiagnostics().m_callbacks, 128);
    EXPECT_EQ(m_world.GetScheduleDiagnostics().m_jobs, 5);
    overlap.Check();
    ASSERT_TRUE(m_world.EndUpdate());

    m_world.BeginUpdate();
    EntityUpdateContext next{ m_world, nullptr, m_world.GetEpoch() };
    Query<const Number>::Create()
        .SetChangeCursor(cursor)
        .SetExecutionPolicy(ExecutionPolicy::kParallel)
        .Traverse(next, Phases::Update, [](const Number&) {
            ADD_FAILURE() << "A row-range write must publish its cursor only after all ranges join";
        });
    ASSERT_TRUE(m_world.SchedulePhase(Phases::Update));
    ASSERT_TRUE(m_world.ExecuteSchedule());
    EXPECT_EQ(m_world.GetScheduleDiagnostics().m_callbacks, 0);
    ASSERT_TRUE(m_world.EndUpdate());
}


TEST_F(WorldFixture, SingleTreeSiblingsRunOnDifferentThreadsAfterTheirParent)
{
    EntityCommandList commands(m_world);
    const auto root = commands.CreateEntity(m_registry, {}, kRoot);
    commands.AddComponent(root, Number{ -1 });
    for (uint32_t i = 0; i < 64; ++i)
    {
        const auto child = commands.CreateEntity(m_registry);
        commands.AddComponent(child, Number{ static_cast<int32_t>(i) });
        commands.SetParent(child, root);
    }

    m_world.Submit(std::move(commands));
    ASSERT_TRUE(m_world.CommitBootstrap());
    ASSERT_EQ(m_world.GetChunkCount(), 1);

    ThreadOverlap overlap;
    std::atomic<uint32_t> children = 0;
    m_world.BeginUpdate();
    EntityUpdateContext context{ m_world, nullptr, m_world.GetEpoch() };
    CascadeQuery<Number, Parent<const Number*>>::Create()
        .SetExecutionPolicy(ExecutionPolicy::kParallel)
        .Traverse(context, Phases::Update, [&](Number& number, const Number* parent) {
            if (!parent)
            {
                number.m_value = 1000;
                return;
            }

            EXPECT_EQ(parent->m_value, 1000);
            if (number.m_value == 0 || number.m_value == 32)
                overlap.Meet(number.m_value == 0 ? 0 : 1);

            number.m_value += parent->m_value;
            ++children;
        });
    ASSERT_TRUE(m_world.SchedulePhase(Phases::Update));
    ASSERT_TRUE(m_world.ExecuteSchedule());
    EXPECT_EQ(children.load(), 64);
    EXPECT_EQ(m_world.GetScheduleDiagnostics().m_treeBatches, 4);
    overlap.Check();
    ASSERT_TRUE(m_world.EndUpdate());
}


TEST_F(WorldFixture, CascadeOrderRebuildsAfterReparentAndSlotReuse)
{
    EntityCommandList commands(m_world);
    const auto root = commands.CreateEntity(m_registry, {}, kRoot);
    const auto child = commands.CreateEntity(m_registry, {}, kChild);
    const auto leaf = commands.CreateEntity(m_registry, {}, kOther);
    for (const auto entity : { root, child, leaf })
        commands.AddComponent(entity, Number{ 0 });

    commands.SetParent(child, root);
    commands.SetParent(leaf, child);
    m_world.Submit(std::move(commands));
    ASSERT_TRUE(m_world.CommitBootstrap());

    const auto tick = [&] {
        m_world.BeginUpdate();
        EntityUpdateContext context{ m_world, nullptr, m_world.GetEpoch() };
        CascadeQuery<Number, Parent<const Number*>>::Create()
            .SetExecutionPolicy(ExecutionPolicy::kParallel)
            .Traverse(context, Phases::Update, [](Number& number, const Number* parent) {
                number.m_value = parent ? parent->m_value + 1 : 1;
            });
        EXPECT_TRUE(m_world.SchedulePhase(Phases::Update));
        EXPECT_TRUE(m_world.ExecuteSchedule());
        EXPECT_TRUE(m_world.EndUpdate());
    };
    tick();
    EXPECT_EQ(m_world.Find(kOther)->FindComponent<Number>()->m_value, 3);
    tick();
    EXPECT_EQ(m_world.Find(kOther)->FindComponent<Number>()->m_value, 3);

    EntityCommandList reparent(m_world);
    reparent.SetParent(m_world.Find(kOther)->GetID(), m_world.Find(kRoot)->GetID());
    m_world.Submit(std::move(reparent));
    tick();
    EXPECT_EQ(m_world.Find(kOther)->FindComponent<Number>()->m_value, 2);

    EntityCommandList destroy(m_world);
    destroy.Destroy(m_world.Find(kChild)->GetID());
    m_world.Submit(std::move(destroy));
    tick();
    EXPECT_EQ(m_world.GetScheduleDiagnostics().m_callbacks, 2);

    EntityCommandList create(m_world);
    const auto replacement = create.CreateEntity(m_registry, {}, kChild);
    create.AddComponent(replacement, Number{ 0 });
    create.SetParent(replacement, m_world.Find(kOther)->GetID());
    m_world.Submit(std::move(create));
    tick();
    EXPECT_EQ(m_world.Find(kChild)->FindComponent<Number>()->m_value, 3);
}


TEST_F(WorldFixture, RuntimeInitializationsAndActivationsAreLimitedTo32Entities)
{
    m_world.Components().Register<Hook>();
    EntityCommandList commands(m_world);
    for (uint32_t i = 0; i < 80; ++i)
    {
        const auto entity = commands.CreateEntity(m_registry);
        commands.AddComponent(entity, Hook{ static_cast<int32_t>(i) });
    }

    m_world.Submit(std::move(commands));
    uint32_t totalActivated = 0;
    for (uint32_t frame = 0; frame < 3; ++frame)
    {
        Hook::s_trace.clear();
        m_world.BeginUpdate();
        ASSERT_TRUE(m_world.Commit());
        ASSERT_TRUE(m_world.EndUpdate());
        uint32_t initialized = 0;
        uint32_t activated = 0;
        for (const int32_t stage : Hook::s_trace)
        {
            initialized += stage % 10 == 2;
            activated += stage % 10 == 3;
        }

        EXPECT_LE(initialized, 32);
        EXPECT_LE(activated, 32);
        totalActivated += activated;
    }

    EXPECT_EQ(totalActivated, 80);
}
