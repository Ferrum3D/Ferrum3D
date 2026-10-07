#include <Core/IO/Artifact.h>
#include <Core/Jobs/Jobs.h>
#include <EntityTestTypes.h>
#include <Framework/Entities/Query.h>
#include <gtest/gtest.h>

using namespace FE;
using namespace FE::Framework;
using namespace FE::Framework::Tests;

namespace
{
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
            auto group = Query<Terms...>::Traverse(context,
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
    };
} // namespace

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


TEST_F(WorldFixture, OverAlignedOversizedAndUnsupportedRelocation)
{
    EntityCommandList commands(m_world);
    auto token = commands.CreateEntity(m_registry, "big", kRoot);
    ASSERT_TRUE(commands.AddComponent(token, Aligned{}));
    Huge huge;
    huge.m_data[69999] = 81;
    ASSERT_TRUE(commands.AddComponent(token, std::move(huge)));
    EXPECT_FALSE(commands.AddComponent(token, ThrowingMove{}));
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
    EXPECT_FALSE(m_world.CommitBootstrap());
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
    second.Rename(token, "invalid");
    m_world.Submit(std::move(second));
    EXPECT_FALSE(m_world.CommitBootstrap());
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
    auto group =
        Query<Number, const Extra*>::Traverse(context, Phases::Update, [&](Entity& entity, Number& number, const Extra* extra) {
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


TEST_F(WorldFixture, ExplicitPrerequisitesOverrideCollectionOrderAndEmptyWorkCompletes)
{
    Entity& entity = Spawn();
    m_world.BeginUpdate();
    EntityUpdateContext context{ m_world, nullptr, m_world.GetEpoch() };
    auto reset = Query<Number>::Traverse(context, Phases::PreUpdate, [](Number& n) {
        n.m_value = 1;
    });
    auto add = Query<Number>::Traverse(context, Phases::PostUpdate, { reset }, [](Number& n) {
        n.m_value += 4;
    });
    auto empty = Query<const Extra>::Traverse(context, Phases::PostUpdate, { add }, [](const Extra&) {
        ADD_FAILURE();
    });
    ASSERT_TRUE(m_world.SchedulePhase(Phases::PreUpdate));
    ASSERT_TRUE(m_world.SchedulePhase(Phases::PostUpdate));
    ASSERT_TRUE(m_world.ExecuteSchedule());
    EXPECT_EQ(entity.FindComponent<Number>()->m_value, 5);
    EXPECT_TRUE(empty->IsSignaled());
    ASSERT_TRUE(m_world.EndUpdate());
}


TEST_F(WorldFixture, InvalidSchedulesExecuteNoCallbacks)
{
    Spawn();
    int calls = 0;
    for (int scenario = 0; scenario < 3; ++scenario)
    {
        m_world.BeginUpdate();
        EntityUpdateContext context{ m_world, nullptr, m_world.GetEpoch() };
        auto first = Query<Number>::Traverse(context, Phases::PostUpdate, [&](Number&) {
            ++calls;
        });
        if (scenario == 0)
            m_world.SchedulePhase(Phases::PreUpdate);
        else if (scenario == 1)
        {
            m_world.SchedulePhase(Phases::PostUpdate);
            EXPECT_FALSE(m_world.SchedulePhase(Phases::PostUpdate));
        }
        else
        {
            Query<Number>::Traverse(context, Phases::PreUpdate, { first }, [&](Number&) {
                ++calls;
            });
            m_world.SchedulePhase(Phases::PreUpdate);
            m_world.SchedulePhase(Phases::PostUpdate);
        }
        EXPECT_FALSE(m_world.ExecuteSchedule());
        EXPECT_EQ(calls, 0);
        EXPECT_FALSE(m_world.EndUpdate());
        EXPECT_TRUE(first->IsSignaled());
    }
}


TEST_F(WorldFixture, PhaseCannotDependOnWorkItHasNotScheduledYet)
{
    Spawn();
    m_world.BeginUpdate();
    EntityUpdateContext context{ m_world, nullptr, m_world.GetEpoch() };
    auto group = Query<Number>::Traverse(context, Phases::Update, [](Number&) {
        ADD_FAILURE();
    });
    Rc<WaitGroup> prerequisite[] = { group };
    m_world.SchedulePhase(Phases::Update, prerequisite);
    EXPECT_FALSE(m_world.ExecuteSchedule());
    EXPECT_FALSE(m_world.EndUpdate());
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
            Query<Number>::Traverse(context, Phases::PostUpdate, [](Number& number) {
                ++number.m_value;
            });
            Query<Number>::Traverse(context, Phases::PreUpdate, [](Number& number) {
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
    auto later = Query<Number>::Traverse(context, Phases::PostUpdate, [](Number& n) {
        n.m_value += 5;
    });
    auto earlier = Query<Number>::Traverse(context, Phases::PostUpdate, [](Number& n) {
        n.m_value *= 3;
    });
    ASSERT_TRUE(m_world.AddPrerequisite(*later, earlier));
    constexpr Phase ApplicationStage{ CompileTimeHash("ApplicationStage", 16), "ApplicationStage" };
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
    auto a = Query<Number>::Traverse(context, Phases::Update, [](Number&) {
        ADD_FAILURE();
    });
    auto b = Query<Number>::Traverse(context, Phases::Update, { a }, [](Number&) {
        ADD_FAILURE();
    });
    ASSERT_TRUE(m_world.AddPrerequisite(*a, b));
    m_world.SchedulePhase(Phases::Update);
    EXPECT_FALSE(m_world.ExecuteSchedule());
    EXPECT_FALSE(m_world.EndUpdate());
}


TEST_F(WorldFixture, OptionalParentTermsExecuteOnRoots)
{
    Spawn();
    m_world.BeginUpdate();
    EntityUpdateContext context{ m_world, nullptr, m_world.GetEpoch() };
    CascadeQuery<const Number, Parent<const Number*>, Extra*>::Traverse(context,
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


TEST_F(WorldFixture, DeclaredComponentDependenciesRejectMissingAndCyclicLayouts)
{
    Rtti::TypeID afterExtra[] = { Rtti::GetTypeID<Extra>() };
    Rtti::TypeID afterNumber[] = { Rtti::GetTypeID<Number>() };
    ASSERT_TRUE(m_world.Components().Register<Number>(afterExtra));
    EntityCommandList missing(m_world);
    auto token = missing.CreateEntity(m_registry, {}, kRoot);
    missing.AddComponent(token, Number{ 1 });
    m_world.Submit(std::move(missing));
    EXPECT_FALSE(m_world.CommitBootstrap());
    EXPECT_EQ(m_world.GetEntityCount(), 0);
    ASSERT_TRUE(m_world.Components().Register<Extra>(afterNumber));
    EntityCommandList cyclic(m_world);
    auto root = cyclic.CreateEntity(m_registry, {}, kRoot);
    cyclic.AddComponent(root, Number{ 1 });
    cyclic.AddComponent(root, Extra{ 2 });
    m_world.Submit(std::move(cyclic));
    EXPECT_FALSE(m_world.CommitBootstrap());
    EXPECT_EQ(m_world.GetEntityCount(), 0);
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


TEST_F(WorldFixture, SerialAndParallelChunkPoliciesExecute)
{
    Spawn();
    m_world.BeginUpdate();
    EntityUpdateContext context{ m_world, nullptr, m_world.GetEpoch() };
    Query<Number>::Traverse(context, Phases::Update, ExecutionPolicy::kParallelChunks, [](Number& number) {
        ++number.m_value;
    });
    m_world.SchedulePhase(Phases::Update);
    EXPECT_TRUE(m_world.ExecuteSchedule());
    EXPECT_TRUE(m_world.EndUpdate());
    m_world.BeginUpdate();
    EntityUpdateContext next{ m_world, nullptr, m_world.GetEpoch() };
    auto group = Query<Number>::Traverse(next, Phases::Update, ExecutionPolicy::kSequential, [](Number& number) {
        ++number.m_value;
    });
    m_world.SchedulePhase(Phases::Update);
    EXPECT_TRUE(m_world.ExecuteSchedule());
    EXPECT_TRUE(group->IsSignaled());
    EXPECT_TRUE(m_world.EndUpdate());
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
    Query<const Number>::Traverse(context, Phases::Update, callback);
    Query<const Number>::Traverse(context, Phases::Update, callback);
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
    auto read = Query<const Number>::Traverse(context, Phases::Update, [](const Number& value) {
        EXPECT_EQ(value.m_value, 42);
    });
    auto write = Query<Number>::Traverse(context, Phases::Update, [](Number& value) {
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
    Query<const Huge, Number>::Traverse(context,
                                        Phases::Update,
                                        ExecutionPolicy::kParallelChunks,
                                        [&](Entity& entity, const Huge&, Number& number) {
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
    CascadeQuery<Extra, Parent<const Number>>::Traverse(context, Phases::Update, [&](Extra& extra, const Number& parent) {
        ++required;
        extra.m_value += parent.m_value;
    });
    CascadeQuery<const Extra, Parent<const Number*>>::Traverse(context,
                                                               Phases::Update,
                                                               [&](Entity& entity, const Extra&, const Number* parent) {
                                                                   ++optional;
                                                                   EXPECT_EQ(parent != nullptr, entity.GetUuid() == kChild);
                                                               });
    ASSERT_TRUE(m_world.SchedulePhase(Phases::Update));
    ASSERT_TRUE(m_world.ExecuteSchedule());
    EXPECT_EQ(required, 1);
    EXPECT_EQ(optional, 2);
    ASSERT_TRUE(m_world.EndUpdate());
}


TEST_F(WorldFixture, CascadeParentCompletesBeforeChildForSerialAndParallelTrees)
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
    for (auto policy : { ExecutionPolicy::kSequential, ExecutionPolicy::kParallelHierarchyTrees })
    {
        m_world.BeginUpdate();
        EntityUpdateContext context{ m_world, nullptr, m_world.GetEpoch() };
        CascadeQuery<Number, Parent<const Number*>>::Traverse(context,
                                                              Phases::Update,
                                                              policy,
                                                              [](Number& number, const Number* parent) {
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
    CascadeQuery<Number>::Traverse(context, Phases::Update, ExecutionPolicy::kParallelHierarchyTrees, [&](Number& number) {
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
            auto callback = [&](const auto&) {
                const uint32_t previous = m_inside.fetch_add(1);
                if (m_policy == ExecutionPolicy::kSequential)
                    EXPECT_EQ(previous, 0);
                else
                {
                    m_entered->Signal();
                    m_entered->Wait();
                }
                m_inside.fetch_sub(1);
            };
            Query<const Number>::Traverse(context, Phases::Update, m_policy, callback);
            Query<const Extra>::Traverse(context, Phases::Update, m_policy, callback);
        }
    } system;
    m_world.AddSystem(system);
    for (auto policy : { ExecutionPolicy::kSequential, ExecutionPolicy::kParallelChunks })
    {
        system.m_policy = policy;
        system.m_entered = WaitGroup::Create(2);
        m_world.BeginUpdate();
        ASSERT_TRUE(m_world.SchedulePhase(Phases::Update));
        ASSERT_TRUE(m_world.ExecuteSchedule());
        EXPECT_EQ(m_world.GetScheduleDiagnostics().m_systemEdges, policy == ExecutionPolicy::kSequential ? 1 : 0);
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
        Query<const Number>::TraverseChanged(context, Phases::Update, cursor, [&](const Number&) {
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


TEST_F(WorldFixture, CascadeChunkPolicyIsRejectedBeforeCallbacks)
{
    Spawn();
    m_world.BeginUpdate();
    EntityUpdateContext context{ m_world, nullptr, m_world.GetEpoch() };
    CascadeQuery<Number>::Traverse(context, Phases::Update, ExecutionPolicy::kParallelChunks, [](Number&) {
        ADD_FAILURE();
    });
    ASSERT_TRUE(m_world.SchedulePhase(Phases::Update));
    EXPECT_FALSE(m_world.ExecuteSchedule());
    EXPECT_FALSE(m_world.EndUpdate());
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
    Query<Number>::Traverse(context, Phases::Update, [&](Number& number) {
        entered->Signal();
        release->Wait();
        number.m_value = 99;
    });
    Query<const Number>::Traverse(context, Phases::Update, [&](const Number& number) {
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
            Query<Number>::Traverse(context, Phases::PreUpdate, [](Number& number) {
                ++number.m_value;
            });
            EXPECT_TRUE(m_world.SchedulePhase(Phases::PreUpdate));
        }
        uint32_t count = 0;
        Query<const Number>::TraverseChanged(context, Phases::Update, first, [&](const Number&) {
            ++count;
        });
        Query<const Number>::TraverseChanged(context, Phases::Update, second, [&](const Number&) {
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


TEST_F(WorldFixture, SharedChangeCursorAndAliasingParallelParentTermsAreRejected)
{
    Spawn();
    ChangeCursor changes;
    m_world.BeginUpdate();
    EntityUpdateContext context{ m_world, nullptr, m_world.GetEpoch() };
    Query<const Number>::TraverseChanged(context, Phases::Update, changes, [](const Number&) {
        ADD_FAILURE();
    });
    Query<const Number>::TraverseChanged(context, Phases::Update, changes, [](const Number&) {
        ADD_FAILURE();
    });
    ASSERT_TRUE(m_world.SchedulePhase(Phases::Update));
    EXPECT_FALSE(m_world.ExecuteSchedule());
    EXPECT_FALSE(m_world.EndUpdate());
    m_world.BeginUpdate();
    EntityUpdateContext next{ m_world, nullptr, m_world.GetEpoch() };
    Query<Number, Parent<const Number*>>::Traverse(next,
                                                   Phases::Update,
                                                   ExecutionPolicy::kParallelChunks,
                                                   [](Number&, const Number*) {
                                                       ADD_FAILURE();
                                                   });
    ASSERT_TRUE(m_world.SchedulePhase(Phases::Update));
    EXPECT_FALSE(m_world.ExecuteSchedule());
    EXPECT_FALSE(m_world.EndUpdate());
}
