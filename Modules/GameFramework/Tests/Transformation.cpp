#include <GameFramework/TransformationSystem.h>
#include <gtest/gtest.h>

using namespace FE;
using namespace FE::Framework;
using namespace FE::GameFramework;

namespace
{
    struct TransformationFixture : testing::Test
    {
        EntityWorld m_world;
        EntityRegistry& m_registry = m_world.CreateRegistry();
        TransformationSystem m_system;
        void SetUp() override
        {
            m_world.AddSystem(m_system);
        }


        void TearDown() override
        {
            m_world.RemoveSystem(m_system);
        }


        void Tick()
        {
            m_world.BeginUpdate();
            ASSERT_TRUE(m_world.SchedulePhase(FE::GameFramework::Phases::Transformation));
            ASSERT_TRUE(m_world.ExecuteSchedule());
            ASSERT_TRUE(m_world.EndUpdate());
        }


        EntityToken Add(EntityCommandList& commands, Uuid uuid, Transform local)
        {
            auto token = commands.CreateEntity(m_registry, {}, uuid);
            commands.AddComponent(token, TransformComponent{ local });
            commands.AddComponent<WorldTransformComponent>(token);
            return token;
        }
    };


    const Uuid kParent("a8cdb00d-20ce-4780-b08d-211d0f610001");
    const Uuid kChild("a8cdb00d-20ce-4780-b08d-211d0f610002");
} // namespace

TEST_F(TransformationFixture, RowVectorTransformScaleAndParentComposition)
{
    EntityCommandList commands(m_world);
    const Transform parentLocal = Transform::Create(Vector3(10, 20, 30), Quaternion::RotationZ(0.4f), 2.0f);
    const Transform childLocal = Transform::Create(Vector3(1, 2, 3), Quaternion::RotationZ(0.2f), 0.5f);
    auto parent = Add(commands, kParent, parentLocal);
    auto child = Add(commands, kChild, childLocal);
    commands.AddComponent(child, NonUniformScaleComponent{ Vector3(2, 3, 4) });
    commands.SetParent(child, parent, ReparentMode::kPreserveLocal);
    m_world.Submit(std::move(commands));
    ASSERT_TRUE(m_world.CommitBootstrap());
    Tick();
    const auto* output = m_world.Find(kChild)->FindComponent<WorldTransformComponent>();
    const Matrix4x4 expected =
        Matrix4x4::Scale(Vector3(2, 3, 4)) * Transform::ToMatrix(childLocal) * Transform::ToMatrix(parentLocal);
    for (uint32_t i = 0; i < 16; ++i)
        EXPECT_NEAR(output->m_world.m_values[i], expected.m_values[i], 0.0001f);
    Tick();
    EXPECT_EQ(m_world.GetScheduleDiagnostics().m_callbacks, 0);
}


TEST_F(TransformationFixture, ParentChangesPropagateAndMissingTransformNodesContributeIdentity)
{
    EntityCommandList commands(m_world);
    auto parent = Add(commands, kParent, Transform::Translation(Vector3(10, 0, 0)));
    auto child = Add(commands, kChild, Transform::Translation(Vector3(1, 0, 0)));
    commands.SetParent(child, parent, ReparentMode::kPreserveLocal);
    m_world.Submit(std::move(commands));
    ASSERT_TRUE(m_world.CommitBootstrap());
    Tick();
    EntityCommandList change(m_world);
    change.ReplaceComponent(m_world.Find(kParent)->GetID(), TransformComponent{ Transform::Translation(Vector3(20, 0, 0)) });
    m_world.Submit(std::move(change));
    Tick();
    EXPECT_FLOAT_EQ(m_world.Find(kChild)->FindComponent<WorldTransformComponent>()->m_world.m_30, 21.0f);
    EntityCommandList remove(m_world);
    remove.RemoveComponent<WorldTransformComponent>(m_world.Find(kParent)->GetID());
    m_world.Submit(std::move(remove));
    Tick();
    EXPECT_FLOAT_EQ(m_world.Find(kChild)->FindComponent<WorldTransformComponent>()->m_world.m_30, 1.0f);
}


TEST_F(TransformationFixture, DeepChainAndLateChildrenPropagateOptionalScaleChanges)
{
    constexpr uint32_t kDepth = 256;
    EntityCommandList commands(m_world);
    auto parent = Add(commands, kParent, Transform::Translation(Vector3(1, 0, 0)));
    for (uint32_t i = 1; i < kDepth; ++i)
    {
        auto child = Add(commands, i == kDepth - 1 ? kChild : Uuid::kNull, Transform::Translation(Vector3(1, 0, 0)));
        commands.SetParent(child, parent, ReparentMode::kPreserveLocal);
        parent = child;
    }
    m_world.Submit(std::move(commands));
    ASSERT_TRUE(m_world.CommitBootstrap());
    Tick();
    EXPECT_FLOAT_EQ(m_world.Find(kChild)->FindComponent<WorldTransformComponent>()->m_world.m_30, float(kDepth));
    EntityCommandList add(m_world);
    auto child = Add(add, Uuid::kNull, Transform::Translation(Vector3(1, 0, 0)));
    add.SetParent(child, m_world.Find(kChild)->GetID(), ReparentMode::kPreserveLocal);
    add.AddComponent(m_world.Find(kParent)->GetID(), NonUniformScaleComponent{ Vector3(2, 2, 2) });
    m_world.Submit(std::move(add));
    Tick();
    EXPECT_FLOAT_EQ(m_world.Find(kChild)->FindComponent<WorldTransformComponent>()->m_world.m_30, 1 + float(kDepth - 1) * 2);
    EXPECT_EQ(m_world.GetEntityCount(), kDepth + 1);
    Tick();
    EXPECT_EQ(m_world.GetScheduleDiagnostics().m_callbacks, 0);
}


TEST_F(TransformationFixture, MixedScaleHierarchiesUpdateWithOptionalScale)
{
    Uuid ids[2][4];
    bool scaled[2][4];
    Vector3 scales[2][4];
    EntityCommandList commands(m_world);
    for (uint32_t tree = 0; tree < 2; ++tree)
    {
        EntityToken parent;
        for (uint32_t depth = 0; depth < 4; ++depth)
        {
            ids[tree][depth] = Uuid::Random();
            auto child = Add(commands, ids[tree][depth], Transform::Translation(Vector3(1, 0, 0)));
            scaled[tree][depth] = (tree + depth) % 2 == 0;
            scales[tree][depth] = Vector3(2, 3, 4);
            if (scaled[tree][depth])
                commands.AddComponent(child, NonUniformScaleComponent{ scales[tree][depth] });

            if (depth != 0)
                commands.SetParent(child, parent, ReparentMode::kPreserveLocal);

            parent = child;
        }
    }
    m_world.Submit(std::move(commands));
    ASSERT_TRUE(m_world.CommitBootstrap());

    auto verify = [&] {
        for (uint32_t tree = 0; tree < 2; ++tree)
        {
            Matrix4x4 parent = Matrix4x4::kIdentity;
            for (uint32_t depth = 0; depth < 4; ++depth)
            {
                Matrix4x4 local = Transform::ToMatrix(Transform::Translation(Vector3(1, 0, 0)));
                if (scaled[tree][depth])
                    local = Matrix4x4::Scale(scales[tree][depth]) * local;

                const Matrix4x4 expected = local * parent;
                const auto* output = m_world.Find(ids[tree][depth])->FindComponent<const WorldTransformComponent>();
                for (uint32_t i = 0; i < 16; ++i)
                    EXPECT_NEAR(output->m_world.m_values[i], expected.m_values[i], 0.0001f);

                parent = expected;
            }
        }
    };
    m_world.BeginUpdate();
    const Rc<WaitGroup> completion[] = { m_system.GetCompletion() };
    EXPECT_FALSE(completion[0]->IsSignaled());
    ASSERT_TRUE(m_world.SchedulePhase(FE::GameFramework::Phases::Transformation));
    m_world.ScheduleStage(FE::Framework::Phases::PostUpdate, verify, completion);
    ASSERT_TRUE(m_world.ExecuteSchedule());
    EXPECT_TRUE(completion[0]->IsSignaled());
    ASSERT_TRUE(m_world.EndUpdate());
    EXPECT_EQ(m_world.GetScheduleDiagnostics().m_callbacks, 8);
    Tick();
    EXPECT_EQ(m_world.GetScheduleDiagnostics().m_callbacks, 0);

    scales[0][0] = Vector3(3, 4, 5);
    EntityCommandList change(m_world);
    change.ReplaceComponent(m_world.Find(ids[0][0])->GetID(), NonUniformScaleComponent{ scales[0][0] });
    m_world.Submit(std::move(change));
    Tick();
    verify();
    EXPECT_EQ(m_world.GetScheduleDiagnostics().m_callbacks, 8);
    Tick();
    EXPECT_EQ(m_world.GetScheduleDiagnostics().m_callbacks, 0);

    EntityCommandList flip(m_world);
    for (uint32_t tree = 0; tree < 2; ++tree)
    {
        for (uint32_t depth = 0; depth < 4; ++depth)
        {
            const EntityID id = m_world.Find(ids[tree][depth])->GetID();
            if (scaled[tree][depth])
                flip.RemoveComponent<NonUniformScaleComponent>(id);
            else
                flip.AddComponent(id, NonUniformScaleComponent{ scales[tree][depth] });

            scaled[tree][depth] = !scaled[tree][depth];
        }
    }
    m_world.Submit(std::move(flip));
    Tick();
    verify();
    EXPECT_EQ(m_world.GetScheduleDiagnostics().m_callbacks, 8);
    Tick();
    EXPECT_EQ(m_world.GetScheduleDiagnostics().m_callbacks, 0);
}


TEST_F(TransformationFixture, PreserveWorldUsesAuthoredTransformsBeforeFirstTick)
{
    EntityCommandList commands(m_world);
    const Transform local = Transform::Create(Vector3(4, 5, 6), Quaternion::RotationZ(0.3f), 2.0f);
    auto child = Add(commands, kChild, local);
    commands.AddComponent(child, NonUniformScaleComponent{ Vector3(2, 3, 4) });
    auto parent = Add(commands, kParent, Transform::Create(Vector3(9, 8, 7), Quaternion::RotationZ(0.7f), 3.0f));
    commands.SetParent(child, parent);
    m_world.Submit(std::move(commands));
    ASSERT_TRUE(m_world.CommitBootstrap());
    Tick();
    const Matrix4x4 expected = Matrix4x4::Scale(Vector3(2, 3, 4)) * Transform::ToMatrix(local);
    const auto* output = m_world.Find(kChild)->FindComponent<WorldTransformComponent>();
    for (uint32_t i = 0; i < 16; ++i)
        EXPECT_NEAR(output->m_world.m_values[i], expected.m_values[i], 0.0001f);
    EntityCommandList detach(m_world);
    detach.SetParent(m_world.Find(kChild)->GetID());
    m_world.Submit(std::move(detach));
    Tick();
    output = m_world.Find(kChild)->FindComponent<WorldTransformComponent>();
    for (uint32_t i = 0; i < 16; ++i)
        EXPECT_NEAR(output->m_world.m_values[i], expected.m_values[i], 0.0001f);
}


TEST_F(TransformationFixture, SingularParentRejectsWholeCommandList)
{
    EntityCommandList commands(m_world);
    Add(commands, kChild, Transform::Translation(Vector3(4, 5, 6)));
    Add(commands, kParent, Transform::Scale(0));
    m_world.Submit(std::move(commands));
    ASSERT_TRUE(m_world.CommitBootstrap());
    Tick();
    EntityCommandList change(m_world);
    change.ReplaceComponent(m_world.Find(kChild)->GetID(), TransformComponent{ Transform::Identity() });
    change.SetParent(m_world.Find(kChild)->GetID(), m_world.Find(kParent)->GetID());
    m_world.Submit(std::move(change));
    m_world.BeginUpdate();
    EXPECT_EQ(m_world.Find(kChild)->GetParent(), nullptr);
    EXPECT_FLOAT_EQ(m_world.Find(kChild)->FindComponent<TransformComponent>()->m_local.Translation().x, 4);
    EXPECT_FALSE(m_world.GetLastError().empty());
    ASSERT_TRUE(m_world.SchedulePhase(FE::GameFramework::Phases::Transformation));
    ASSERT_TRUE(m_world.ExecuteSchedule());
    ASSERT_TRUE(m_world.EndUpdate());
}


TEST_F(TransformationFixture, PlacementRootAndCookedChildrenReceiveRuntimeOutputs)
{
    const Transform childLocal = Transform::Create(Vector3(1, 2, 3), Quaternion::RotationZ(0.2f), 0.5f);
    const Transform rootLocal = Transform::Create(Vector3(10, 20, 30), Quaternion::RotationZ(0.4f), 2.0f);
    EntityCollection collection;
    EntityRecord child;
    child.m_uuid = kChild;
    ASSERT_TRUE(collection.CookComponent(child, TransformComponent{ childLocal }));
    collection.m_entities.push_back(std::move(child));
    EntityCollectionInstanceAsset placement;
    placement.m_collection = IO::Link<EntityCollection>(kParent);
    ASSERT_TRUE(placement.UpdateBindings(collection));
    ASSERT_TRUE(placement.m_root.CookComponent(placement.m_root.m_entities.front(), TransformComponent{ rootLocal }));
    const auto operation = m_world.LoadPlacement(m_registry, kParent, placement, collection);
    Tick();
    ASSERT_EQ(m_world.GetMaterializationStatus(operation).m_state, MaterializationState::kReady);
    auto* entity = m_world.Find(placement.m_bindings.front().m_entityUuid);
    ASSERT_NE(entity, nullptr);
    const auto* output = entity->FindComponent<const WorldTransformComponent>();
    ASSERT_NE(output, nullptr);
    const Matrix4x4 expected = Transform::ToMatrix(childLocal) * Transform::ToMatrix(rootLocal);
    for (uint32_t i = 0; i < 16; ++i)
        EXPECT_NEAR(output->m_world.m_values[i], expected.m_values[i], 0.0001f);
    EXPECT_EQ(entity->GetParent()->GetUuid(), placement.m_rootUuid);
}


TEST_F(TransformationFixture, PreserveWorldHonorsPrecedingLocalEditsAndExplicitLocalReparent)
{
    EntityCommandList commands(m_world);
    Add(commands, kChild, Transform::Translation(Vector3(1, 0, 0)));
    Add(commands, kParent, Transform::Translation(Vector3(10, 0, 0)));
    m_world.Submit(std::move(commands));
    ASSERT_TRUE(m_world.CommitBootstrap());
    Tick();
    EntityCommandList reparent(m_world);
    reparent.ReplaceComponent(m_world.Find(kChild)->GetID(), TransformComponent{ Transform::Translation(Vector3(7, 0, 0)) });
    reparent.SetParent(m_world.Find(kChild)->GetID(), m_world.Find(kParent)->GetID());
    m_world.Submit(std::move(reparent));
    Tick();
    EXPECT_FLOAT_EQ(m_world.Find(kChild)->FindComponent<const WorldTransformComponent>()->m_world.m_30, 7);
    EntityCommandList detach(m_world);
    detach.SetParent(m_world.Find(kChild)->GetID(), {}, ReparentMode::kPreserveLocal);
    m_world.Submit(std::move(detach));
    Tick();
    EXPECT_FLOAT_EQ(m_world.Find(kChild)->FindComponent<const WorldTransformComponent>()->m_world.m_30, -3);
}


TEST_F(TransformationFixture, ManyShallowTreesPublishEveryDerivedTransform)
{
    EntityCommandList commands(m_world);
    festd::vector<Uuid> children;
    for (uint32_t tree = 0; tree < 64; ++tree)
    {
        const auto uuid = Uuid::Random();
        children.push_back(uuid);
        auto parent = Add(commands, Uuid::kNull, Transform::Translation(Vector3(float(tree), 0, 0)));
        auto child = Add(commands, uuid, Transform::Translation(Vector3(1, 0, 0)));
        commands.SetParent(child, parent, ReparentMode::kPreserveLocal);
    }
    m_world.Submit(std::move(commands));
    ASSERT_TRUE(m_world.CommitBootstrap());
    Tick();
    EXPECT_EQ(m_world.GetScheduleDiagnostics().m_treeBatches, 4);
    for (uint32_t tree = 0; tree < children.size(); ++tree)
        EXPECT_FLOAT_EQ(m_world.Find(children[tree])->FindComponent<const WorldTransformComponent>()->m_world.m_30,
                        float(tree + 1));
    Tick();
    EXPECT_EQ(m_world.GetScheduleDiagnostics().m_callbacks, 0);
}


TEST_F(TransformationFixture, PreserveWorldRetainsAdjustedLocalUnderInactiveParent)
{
    EntityCommandList setup(m_world);
    Add(setup, kParent, Transform::Translation(Vector3(10, 0, 0)));
    Add(setup, kChild, Transform::Translation(Vector3(7, 0, 0)));
    m_world.Submit(std::move(setup));
    ASSERT_TRUE(m_world.CommitBootstrap());
    Tick();
    Entity* parent = m_world.Find(kParent);
    Entity* child = m_world.Find(kChild);

    EntityCommandList deactivate(m_world);
    deactivate.SetActive(parent->GetID(), false);
    m_world.Submit(std::move(deactivate));
    Tick();
    EntityCommandList reparent(m_world);
    reparent.SetParent(child->GetID(), parent->GetID());
    m_world.Submit(std::move(reparent));
    Tick();
    EXPECT_EQ(child->GetParent(), parent);
    EXPECT_FALSE(child->IsActive());
    EXPECT_FLOAT_EQ(child->FindComponent<TransformComponent>()->m_local.Translation().x, -3);

    EntityCommandList reactivate(m_world);
    reactivate.SetActive(parent->GetID(), true);
    m_world.Submit(std::move(reactivate));
    Tick();
    EXPECT_TRUE(child->IsActive());
    EXPECT_FLOAT_EQ(child->FindComponent<WorldTransformComponent>()->m_world.m_30, 7);
}


TEST_F(TransformationFixture, PreserveWorldKeepsUniformScaleInLocalTransform)
{
    EntityCommandList setup(m_world);
    const Transform original = Transform::Create(Vector3(4, 5, 6), Quaternion::RotationZ(0.3f), 2.0f);
    auto child = Add(setup, kChild, original);
    auto parent = Add(setup, kParent, Transform::Create(Vector3(9, 8, 7), Quaternion::RotationZ(0.7f), 4.0f));
    setup.SetParent(child, parent);
    m_world.Submit(std::move(setup));
    ASSERT_TRUE(m_world.CommitBootstrap());
    Tick();

    const auto* entity = m_world.Find(kChild);
    EXPECT_NEAR(entity->FindComponent<TransformComponent>()->m_local.Scale(), 0.5f, 0.00001f);
    const Matrix4x4 expected = Transform::ToMatrix(original);
    const auto* output = entity->FindComponent<WorldTransformComponent>();
    for (uint32_t i = 0; i < 16; ++i)
        EXPECT_NEAR(output->m_world.m_values[i], expected.m_values[i], 0.0001f);
}


TEST_F(TransformationFixture, PreserveWorldDecomposesNonUniformScaleIntoModifier)
{
    EntityCommandList setup(m_world);
    auto child = Add(setup, kChild, Transform::Translation(Vector3(4, 5, 6)));
    setup.AddComponent<NonUniformScaleComponent>(child);
    auto parent = Add(setup, kParent, Transform::Translation(Vector3(9, 8, 7)));
    setup.AddComponent(parent, NonUniformScaleComponent{ Vector3(2, 3, 4) });
    setup.SetParent(child, parent);
    m_world.Submit(std::move(setup));
    ASSERT_TRUE(m_world.CommitBootstrap());
    Tick();

    const auto* entity = m_world.Find(kChild);
    const auto* modifier = entity->FindComponent<NonUniformScaleComponent>();
    EXPECT_NEAR(modifier->m_scale.x, 0.5f, 0.00001f);
    EXPECT_NEAR(modifier->m_scale.y, 1.0f / 3.0f, 0.00001f);
    EXPECT_NEAR(modifier->m_scale.z, 0.25f, 0.00001f);
    EXPECT_FLOAT_EQ(entity->FindComponent<TransformComponent>()->m_local.Scale(), 1.0f);
    const Matrix4x4 expected = Matrix4x4::Translation(Vector3(4, 5, 6));
    const auto* output = entity->FindComponent<WorldTransformComponent>();
    for (uint32_t i = 0; i < 16; ++i)
        EXPECT_NEAR(output->m_world.m_values[i], expected.m_values[i], 0.0001f);
}


TEST_F(TransformationFixture, PreserveWorldRejectsShearWithoutPublishingLocalEdits)
{
    EntityCommandList setup(m_world);
    auto child = Add(setup, kChild, Transform::Translation(Vector3(4, 5, 6)));
    setup.AddComponent<NonUniformScaleComponent>(child);
    auto parent = Add(setup, kParent, Transform::Identity());
    setup.AddComponent(parent, NonUniformScaleComponent{ Vector3(2, 3, 4) });
    m_world.Submit(std::move(setup));
    ASSERT_TRUE(m_world.CommitBootstrap());
    Tick();

    Entity* entity = m_world.Find(kChild);
    EntityCommandList change(m_world);
    change.ReplaceComponent(entity->GetID(), TransformComponent{ Transform::Rotation(Quaternion::RotationZ(0.4f)) });
    change.SetParent(entity->GetID(), m_world.Find(kParent)->GetID());
    m_world.Submit(std::move(change));
    Tick();

    EXPECT_EQ(entity->GetParent(), nullptr);
    EXPECT_FLOAT_EQ(entity->FindComponent<TransformComponent>()->m_local.Translation().x, 4);
    EXPECT_TRUE(Math::CmpEqual(entity->FindComponent<NonUniformScaleComponent>()->m_scale, Vector3(1, 1, 1)));
    EXPECT_FALSE(m_world.GetLastError().empty());
}


TEST_F(TransformationFixture, PreserveWorldRejectsNonUniformScaleWithoutModifier)
{
    EntityCommandList setup(m_world);
    Add(setup, kChild, Transform::Translation(Vector3(4, 5, 6)));
    auto parent = Add(setup, kParent, Transform::Identity());
    setup.AddComponent(parent, NonUniformScaleComponent{ Vector3(2, 3, 4) });
    m_world.Submit(std::move(setup));
    ASSERT_TRUE(m_world.CommitBootstrap());
    Tick();

    Entity* entity = m_world.Find(kChild);
    EntityCommandList change(m_world);
    change.SetParent(entity->GetID(), m_world.Find(kParent)->GetID());
    m_world.Submit(std::move(change));
    Tick();

    EXPECT_EQ(entity->GetParent(), nullptr);
    EXPECT_FLOAT_EQ(entity->FindComponent<TransformComponent>()->m_local.Translation().x, 4);
    EXPECT_FALSE(m_world.GetLastError().empty());
}
