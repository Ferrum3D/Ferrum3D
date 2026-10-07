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
        EntityToken Add(EntityCommandList& commands, Uuid uuid, Matrix4x4 local)
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

TEST_F(TransformationFixture, RowVectorAffineScaleAndParentComposition)
{
    EntityCommandList commands(m_world);
    const Matrix4x4 parentLocal = Matrix4x4::RotationZ(0.4f) * Matrix4x4::Translation(Vector3(10, 20, 30));
    Matrix4x4 childLocal = Matrix4x4::Translation(Vector3(1, 2, 3));
    childLocal.m_01 = 0.25f;
    auto parent = Add(commands, kParent, parentLocal);
    auto child = Add(commands, kChild, childLocal);
    commands.AddComponent(child, NonUniformScaleComponent{ Vector3(2, 3, 4) });
    commands.SetParent(child, parent, ReparentMode::kPreserveLocal);
    m_world.Submit(std::move(commands));
    ASSERT_TRUE(m_world.CommitBootstrap());
    Tick();
    const auto* output = m_world.Find(kChild)->FindComponent<WorldTransformComponent>();
    const Matrix4x4 expected = Matrix4x4::Scale(Vector3(2, 3, 4)) * childLocal * parentLocal;
    for (uint32_t i = 0; i < 16; ++i)
        EXPECT_NEAR(output->m_world.m_values[i], expected.m_values[i], 0.0001f);
    Tick();
    EXPECT_EQ(m_world.GetScheduleDiagnostics().m_callbacks, 0);
}


TEST_F(TransformationFixture, ParentChangesPropagateAndMissingTransformNodesContributeIdentity)
{
    EntityCommandList commands(m_world);
    auto parent = Add(commands, kParent, Matrix4x4::Translation(Vector3(10, 0, 0)));
    auto child = Add(commands, kChild, Matrix4x4::Translation(Vector3(1, 0, 0)));
    commands.SetParent(child, parent, ReparentMode::kPreserveLocal);
    m_world.Submit(std::move(commands));
    ASSERT_TRUE(m_world.CommitBootstrap());
    Tick();
    EntityCommandList change(m_world);
    change.ReplaceComponent(m_world.Find(kParent)->GetID(), TransformComponent{ Matrix4x4::Translation(Vector3(20, 0, 0)) });
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
    auto parent = Add(commands, kParent, Matrix4x4::Translation(Vector3(1, 0, 0)));
    for (uint32_t i = 1; i < kDepth; ++i)
    {
        auto child = Add(commands, i == kDepth - 1 ? kChild : Uuid::kNull, Matrix4x4::Translation(Vector3(1, 0, 0)));
        commands.SetParent(child, parent, ReparentMode::kPreserveLocal);
        parent = child;
    }
    m_world.Submit(std::move(commands));
    ASSERT_TRUE(m_world.CommitBootstrap());
    Tick();
    EXPECT_FLOAT_EQ(m_world.Find(kChild)->FindComponent<WorldTransformComponent>()->m_world.m_30, float(kDepth));
    EntityCommandList add(m_world);
    auto child = Add(add, Uuid::kNull, Matrix4x4::Translation(Vector3(1, 0, 0)));
    add.SetParent(child, m_world.Find(kChild)->GetID(), ReparentMode::kPreserveLocal);
    add.AddComponent(m_world.Find(kParent)->GetID(), NonUniformScaleComponent{ Vector3(2, 2, 2) });
    m_world.Submit(std::move(add));
    Tick();
    EXPECT_FLOAT_EQ(m_world.Find(kChild)->FindComponent<WorldTransformComponent>()->m_world.m_30, 1 + float(kDepth - 1) * 2);
    EXPECT_EQ(m_world.GetEntityCount(), kDepth + 1);
    Tick();
    EXPECT_EQ(m_world.GetScheduleDiagnostics().m_callbacks, 0);
}


TEST_F(TransformationFixture, PreserveWorldUsesAuthoredAffineValuesBeforeFirstTick)
{
    EntityCommandList commands(m_world);
    Matrix4x4 local = Matrix4x4::RotationZ(0.3f) * Matrix4x4::Translation(Vector3(4, 5, 6));
    local.m_01 += 0.4f;
    auto child = Add(commands, kChild, local);
    commands.AddComponent(child, NonUniformScaleComponent{ Vector3(2, 3, 4) });
    auto parent = Add(commands, kParent, Matrix4x4::Scale(Vector3(3, 2, 1)) * Matrix4x4::Translation(Vector3(9, 8, 7)));
    commands.SetParent(child, parent);
    m_world.Submit(std::move(commands));
    ASSERT_TRUE(m_world.CommitBootstrap());
    Tick();
    const Matrix4x4 expected = Matrix4x4::Scale(Vector3(2, 3, 4)) * local;
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
    Add(commands, kChild, Matrix4x4::Translation(Vector3(4, 5, 6)));
    Add(commands, kParent, Matrix4x4::Scale(Vector3(0, 2, 3)));
    m_world.Submit(std::move(commands));
    ASSERT_TRUE(m_world.CommitBootstrap());
    Tick();
    EntityCommandList change(m_world);
    change.ReplaceComponent(m_world.Find(kChild)->GetID(), TransformComponent{ Matrix4x4::kIdentity });
    change.SetParent(m_world.Find(kChild)->GetID(), m_world.Find(kParent)->GetID());
    m_world.Submit(std::move(change));
    m_world.BeginUpdate();
    EXPECT_EQ(m_world.Find(kChild)->GetParent(), nullptr);
    EXPECT_FLOAT_EQ(m_world.Find(kChild)->FindComponent<TransformComponent>()->m_local.m_30, 4);
    EXPECT_FALSE(m_world.GetLastError().empty());
    ASSERT_TRUE(m_world.SchedulePhase(FE::GameFramework::Phases::Transformation));
    ASSERT_TRUE(m_world.ExecuteSchedule());
    ASSERT_TRUE(m_world.EndUpdate());
}


TEST_F(TransformationFixture, PlacementRootAndCookedChildrenReceiveRuntimeOutputs)
{
    EntityCollection collection;
    EntityRecord child;
    child.m_uuid = kChild;
    ASSERT_TRUE(collection.CookComponent(child, TransformComponent{ Matrix4x4::Translation(Vector3(1, 2, 3)) }));
    collection.m_entities.push_back(std::move(child));
    EntityCollectionInstanceAsset placement;
    placement.m_collection = IO::Link<EntityCollection>(kParent);
    ASSERT_TRUE(placement.UpdateBindings(collection));
    ASSERT_TRUE(placement.m_root.CookComponent(placement.m_root.m_entities.front(),
                                               TransformComponent{ Matrix4x4::Translation(Vector3(10, 20, 30)) }));
    const auto operation = m_world.LoadPlacement(m_registry, kParent, placement, collection);
    Tick();
    ASSERT_EQ(m_world.GetMaterializationStatus(operation).m_state, MaterializationState::kReady);
    auto* entity = m_world.Find(placement.m_bindings.front().m_entityUuid);
    ASSERT_NE(entity, nullptr);
    const auto* output = entity->FindComponent<const WorldTransformComponent>();
    ASSERT_NE(output, nullptr);
    EXPECT_FLOAT_EQ(output->m_world.m_30, 11);
    EXPECT_FLOAT_EQ(output->m_world.m_31, 22);
    EXPECT_FLOAT_EQ(output->m_world.m_32, 33);
    EXPECT_EQ(entity->GetParent()->GetUuid(), placement.m_rootUuid);
}


TEST_F(TransformationFixture, PreserveWorldHonorsPrecedingLocalEditsAndExplicitLocalReparent)
{
    EntityCommandList commands(m_world);
    Add(commands, kChild, Matrix4x4::Translation(Vector3(1, 0, 0)));
    Add(commands, kParent, Matrix4x4::Translation(Vector3(10, 0, 0)));
    m_world.Submit(std::move(commands));
    ASSERT_TRUE(m_world.CommitBootstrap());
    Tick();
    EntityCommandList reparent(m_world);
    reparent.ReplaceComponent(m_world.Find(kChild)->GetID(), TransformComponent{ Matrix4x4::Translation(Vector3(7, 0, 0)) });
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
        const auto uuid = NewEntityUuid();
        children.push_back(uuid);
        auto parent = Add(commands, Uuid::kNull, Matrix4x4::Translation(Vector3(float(tree), 0, 0)));
        auto child = Add(commands, uuid, Matrix4x4::Translation(Vector3(1, 0, 0)));
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
