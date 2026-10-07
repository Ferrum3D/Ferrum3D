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
    commands.SetParent(child, parent);
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
    commands.SetParent(child, parent);
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
        commands.SetParent(child, parent);
        parent = child;
    }
    m_world.Submit(std::move(commands));
    ASSERT_TRUE(m_world.CommitBootstrap());
    Tick();
    EXPECT_FLOAT_EQ(m_world.Find(kChild)->FindComponent<WorldTransformComponent>()->m_world.m_30, float(kDepth));
    EntityCommandList add(m_world);
    auto child = Add(add, Uuid::kNull, Matrix4x4::Translation(Vector3(1, 0, 0)));
    add.SetParent(child, m_world.Find(kChild)->GetID());
    add.AddComponent(m_world.Find(kParent)->GetID(), NonUniformScaleComponent{ Vector3(2, 2, 2) });
    m_world.Submit(std::move(add));
    Tick();
    EXPECT_FLOAT_EQ(m_world.Find(kChild)->FindComponent<WorldTransformComponent>()->m_world.m_30, 1 + float(kDepth - 1) * 2);
    EXPECT_EQ(m_world.GetEntityCount(), kDepth + 1);
    Tick();
    EXPECT_EQ(m_world.GetScheduleDiagnostics().m_callbacks, 0);
}
