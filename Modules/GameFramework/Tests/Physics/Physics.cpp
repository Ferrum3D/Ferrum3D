#include <AssetBuilder/AssetFile.h>
#include <AssetBuilder/AssetPipeline.h>
#include <Core/IO/Artifact.h>
#include <Core/IO/AssetManager.h>
#include <Core/Threading/Thread.h>
#include <Core/Time/BaseTime.h>
#include <Framework/Entities/EntityWorldAsset.h>
#include <Framework/Entities/Query.h>
#include <GameFramework/Components/TransformComponents.h>
#include <GameFramework/Services/WorldPhysicsService.h>
#include <GameFramework/Systems/PhysicsSystem.h>
#include <GameFramework/Systems/TransformationSystem.h>
#include <algorithm>
#include <gtest/gtest.h>

using namespace FE;
using namespace FE::Framework;
using namespace FE::GameFramework;

namespace
{
    struct PhysicsFixture : testing::Test
    {
        EntityWorld m_world;
        WorldPhysicsService m_physics;
        PhysicsSystem m_system;
        TransformationSystem m_transforms;
        EntityRegistry& m_registry = m_world.CreateRegistry();

        void SetUp() override
        {
            m_world.AddService(m_physics);
            m_world.AddSystem(m_system);
            m_world.AddSystem(m_transforms);
        }


        void TearDown() override
        {
            m_world.Clear();
            EXPECT_EQ(m_physics.GetBodyCount(), 0u);
            m_world.RemoveSystem(m_transforms);
            m_world.RemoveSystem(m_system);
            m_world.RemoveService(m_physics);
        }


        void Step(const uint32_t count = 1)
        {
            for (uint32_t i = 0; i < count; ++i)
            {
                m_world.BeginFrame();
                m_world.BeginFixedUpdate(1.0f / 60.0f);
                ASSERT_TRUE(m_world.SchedulePhase(FE::Framework::Phases::PreUpdate));
                ASSERT_TRUE(m_world.SchedulePhase(FE::Framework::Phases::Update));
                ASSERT_TRUE(m_world.SchedulePhase(FE::Framework::Phases::PostUpdate));
                ASSERT_TRUE(m_world.ExecuteSchedule());
                ASSERT_TRUE(m_world.EndUpdate());
            }
        }


        Uuid Add(const Vector3 position, const PhysicsBodyComponent& body, const float scale = 1)
        {
            const Uuid id = Uuid::Random();
            EntityCommandList commands(m_world);
            const auto token = commands.CreateEntity(m_registry, {}, id);
            commands.AddComponent(token, TransformComponent{ Transform::Create(position, Quaternion::kIdentity, scale) });
            commands.AddComponent(token, body);
            m_world.Submit(std::move(commands));
            return id;
        }


        void AddFloor()
        {
            PhysicsBodyComponent floor;
            floor.m_motionType = PhysicsMotionType::kStatic;
            PhysicsBodyShape shape;
            shape.m_shape.m_type = PhysicsShapeType::kBox;
            shape.m_shape.m_halfExtent = Vector3(20, 0.5f, 20);
            floor.m_shapes.push_back(shape);
            Add(Vector3(0, -0.5f, 0), floor);
        }
    };
} // namespace


TEST_F(PhysicsFixture, DynamicSphereFallsAndRestsOnStaticBox)
{
    AddFloor();
    PhysicsBodyComponent sphere;
    sphere.m_shapes.push_back({});
    const auto id = Add(Vector3(0, 5, 0), sphere);
    ASSERT_TRUE(m_world.CommitBootstrap());
    EXPECT_EQ(m_physics.GetBodyCount(), 2u);
    Step(30);
    EXPECT_LT(m_world.Find(id)->FindComponent<const TransformComponent>()->m_local.Translation().y, 4.5f);
    Step(210);
    EXPECT_NEAR(m_world.Find(id)->FindComponent<const TransformComponent>()->m_local.Translation().y, 0.5f, 0.03f);

    m_world.BeginUpdate();
    ASSERT_TRUE(m_world.SchedulePhase(FE::GameFramework::Phases::Transformation));
    ASSERT_TRUE(m_world.ExecuteSchedule());
    ASSERT_TRUE(m_world.EndUpdate());
    EXPECT_NEAR(m_world.Find(id)->FindComponent<const WorldTransformComponent>()->m_world.m_31, 0.5f, 0.03f);
}


TEST_F(PhysicsFixture, CompoundOffsetAndUniformScalePreserveBodyOrigin)
{
    AddFloor();
    PhysicsBodyComponent body;
    PhysicsBodyShape shape;
    shape.m_position = Vector3(-0.6f, 1, 0);
    body.m_shapes.push_back(shape);
    shape.m_position.x = 0.6f;
    body.m_shapes.push_back(shape);
    const auto id = Add(Vector3(0, 6, 0), body, 2);
    ASSERT_TRUE(m_world.CommitBootstrap());
    Step(300);
    const auto& result = m_world.Find(id)->FindComponent<const TransformComponent>()->m_local;
    EXPECT_NEAR(result.Translation().y, -1.0f, 0.08f);
    EXPECT_FLOAT_EQ(result.Scale(), 2);
}


TEST_F(PhysicsFixture, AllPrimitivesAndKinematicTargets)
{
    for (const auto type : { PhysicsShapeType::kBox, PhysicsShapeType::kCapsule, PhysicsShapeType::kCylinder })
    {
        PhysicsBodyComponent body;
        PhysicsBodyShape shape;
        shape.m_shape.m_type = type;
        body.m_shapes.push_back(shape);
        Add(Vector3(static_cast<float>(type) * 3, 4, 0), body);
    }

    PhysicsBodyComponent kinematic;
    kinematic.m_shapes.push_back({});
    kinematic.m_motionType = PhysicsMotionType::kKinematic;
    const auto id = Add(Vector3(0, 5, 0), kinematic);
    ASSERT_TRUE(m_world.CommitBootstrap());
    Step(10);
    EXPECT_FLOAT_EQ(m_world.Find(id)->FindComponent<const TransformComponent>()->m_local.Translation().y, 5);

    EntityCommandList commands(m_world);
    commands.ReplaceComponent(m_world.Find(id)->GetID(), TransformComponent{ Transform::Translation(Vector3(2, 6, 0)) });
    m_world.Submit(std::move(commands));
    Step();
    const auto* runtime = m_world.Find(id)->FindComponent<const PhysicsBodyComponent>();
    EXPECT_NEAR(m_physics.GetTransform(runtime->m_body, 1).Translation().x, 2, 0.001f);
    EXPECT_EQ(m_physics.GetBodyCount(), 4u);
}


TEST_F(PhysicsFixture, InvalidShapesFailWithoutLeakingBodies)
{
    PhysicsBodyComponent empty;
    const auto id = Add(Vector3::kZero, empty);
    ASSERT_TRUE(m_world.CommitBootstrap());
    EXPECT_EQ(m_world.Find(id), nullptr);
    ASSERT_NE(m_world.Find(id, false), nullptr);
    EXPECT_TRUE(m_world.Find(id, false)->HasFailed());
    EXPECT_EQ(m_physics.GetBodyCount(), 0u);
}


TEST_F(PhysicsFixture, PhysicsRegistrationSurvivesAnotherWorldShuttingDown)
{
    EntityWorld otherWorld;
    WorldPhysicsService otherPhysics;
    otherWorld.AddService(otherPhysics);
    otherWorld.RemoveService(otherPhysics);

    PhysicsBodyComponent body;
    body.m_shapes.push_back({});
    const auto id = Add(Vector3(0, 5, 0), body);
    ASSERT_TRUE(m_world.CommitBootstrap());
    Step(10);
    EXPECT_LT(m_world.Find(id)->FindComponent<const TransformComponent>()->m_local.Translation().y, 5);
}


TEST_F(PhysicsFixture, DestructionAndComponentReplacementReleaseBodies)
{
    PhysicsBodyComponent body;
    body.m_shapes.push_back({});
    const auto id = Add(Vector3(0, 5, 0), body);
    ASSERT_TRUE(m_world.CommitBootstrap());
    EntityCommandList replace(m_world);
    body.m_shapes[0].m_shape.m_radius = 1;
    replace.ReplaceComponent(m_world.Find(id)->GetID(), body);
    m_world.Submit(std::move(replace));
    Step();
    EXPECT_EQ(m_physics.GetBodyCount(), 1u);

    EntityCommandList remove(m_world);
    remove.Destroy(m_world.Find(id)->GetID());
    m_world.Submit(std::move(remove));
    Step();
    EXPECT_EQ(m_physics.GetBodyCount(), 0u);
}


TEST_F(PhysicsFixture, SnapshotRestoresMotionWithFreshBodyMembership)
{
    PhysicsBodyComponent body;
    body.m_shapes.push_back({});
    const auto id = Add(Vector3(0, 5, 0), body);
    ASSERT_TRUE(m_world.CommitBootstrap());
    Step(20);
    const auto before = m_world.Find(id)->FindComponent<const PhysicsBodyComponent>()->m_linearVelocity;
    EXPECT_LT(before.y, -2.0f);

    EntityWorldSnapshotAsset snapshot;
    ASSERT_TRUE(m_world.CaptureSnapshot(snapshot));
    m_world.Clear();
    ASSERT_EQ(m_physics.GetBodyCount(), 0u);
    ASSERT_TRUE(m_world.RestoreSnapshot(snapshot));
    Step();
    ASSERT_NE(m_world.Find(id), nullptr);
    EXPECT_EQ(m_physics.GetBodyCount(), 1u);
    EXPECT_LT(m_world.Find(id)->FindComponent<const PhysicsBodyComponent>()->m_linearVelocity.y, before.y);
}


TEST_F(PhysicsFixture, SharedShapeAssetBuildsLoadsAndReleasesWithEntity)
{
    const IO::Path directory = IO::Path(FE_GAME_ASSET_TEST_OUTPUT) / Fmt::Format("Physics-{}", Uuid::Random());
    const IO::Path path = directory / "sphere.asset";
    ASSERT_TRUE(AssetBuilder::ImportAsset({ {}, path, Rtti::GetTypeID<PhysicsShapeAsset>() }));
    AssetBuilder::AssetFile file;
    ASSERT_TRUE(AssetBuilder::LoadAssetFile(path, file));
    PhysicsShapeAsset shape;
    shape.m_radius = 1.25f;
    file.m_artifacts.front().m_buildSettings.Emplace<PhysicsShapeAsset>(shape);
    const IO::AssetID asset = file.m_artifacts.front().m_assetId;
    ASSERT_TRUE(AssetBuilder::SaveAssetFile(path, file));
    ASSERT_TRUE(AssetBuilder::BuildAsset({ path, {}, directory }));

    IO::ArtifactStore::SetCatalogSource(directory);
    IO::AssetManager::Init();
    auto cleanup = festd::defer([&] {
        m_world.Clear();
        IO::AssetManager::Shutdown();
    });

    AddFloor();
    PhysicsBodyComponent body;
    PhysicsBodyShape entry;
    entry.m_asset = IO::Link<PhysicsShapeAsset>(asset);
    body.m_shapes.push_back(entry);
    const auto id = Add(Vector3(0, 4, 0), body);
    for (uint32_t i = 0; i < 5000; ++i)
    {
        IO::AssetManager::Tick();
        ASSERT_TRUE(m_world.CommitBootstrap());
        if (m_world.Find(id))
            break;

        Threading::Sleep(1);
    }

    ASSERT_NE(m_world.Find(id), nullptr);
    Step(240);
    EXPECT_NEAR(m_world.Find(id)->FindComponent<const TransformComponent>()->m_local.Translation().y, 1.25f, 0.03f);
}


TEST_F(PhysicsFixture, ParallelAndSerialStepsCanAlternate)
{
    festd::array<uint32_t, 128> bodies;
    bodies.fill(UINT32_MAX);
    auto cleanup = festd::defer([&] {
        for (const uint32_t body : bodies)
            m_physics.DestroyBody(body);
    });

    PhysicsBodyComponent definition;
    definition.m_shapes.push_back({});
    const Transform start = Transform::Translation(Vector3(0, 10, 0));
    ASSERT_TRUE(m_physics.CreateBody(definition, start, Vector3(1), bodies.front()));
    m_physics.Step(1.0f / 60.0f);

    for (uint32_t i = 1; i < bodies.size(); ++i)
    {
        const Transform transform = Transform::Translation(Vector3(static_cast<float>(i) * 2, 10, 0));
        ASSERT_TRUE(m_physics.CreateBody(definition, transform, Vector3(1), bodies[i]));
    }

    EXPECT_EQ(m_physics.GetBodyCount(), bodies.size());
    for (uint32_t i = 0; i < 8; ++i)
        m_physics.Step(1.0f / 60.0f);

    for (uint32_t i = 1; i < bodies.size(); ++i)
    {
        m_physics.DestroyBody(bodies[i]);
        bodies[i] = UINT32_MAX;
    }

    m_physics.Step(1.0f / 60.0f);
    EXPECT_EQ(m_physics.GetBodyCount(), 1u);
    EXPECT_LT(m_physics.GetTransform(bodies.front(), 1).Translation().y, 10);
}


TEST_F(PhysicsFixture, DISABLED_StepBenchmark)
{
    AddFloor();
    PhysicsBodyComponent body;
    body.m_shapes.push_back({});
    body.m_friction = 0;
    body.m_linearVelocity = Vector3(0, 0, 1);
    for (uint32_t i = 0; i < 10; ++i)
        Add(Vector3(static_cast<float>(i % 5) * 3 - 6, 5, static_cast<float>(i / 5) * 3), body);

    ASSERT_TRUE(m_world.CommitBootstrap());
    for (uint32_t i = 0; i < 32; ++i)
        m_physics.Step(1.0f / 60.0f);

    festd::array<double, 512> samples;
    double total = 0;
    for (double& sample : samples)
    {
        const uint64_t start = Platform::GetTicks();
        m_physics.Step(1.0f / 60.0f);
        sample = static_cast<double>(Platform::GetTicks() - start) * Platform::GetSecondsPerTick() * 1.0e6;
        total += sample;
    }

    std::sort(samples.begin(), samples.end());
    Logger::LogInfo("Physics step (11 bodies): mean {} us, median {} us, p95 {} us, max {} us",
                    total / samples.size(),
                    samples[samples.size() / 2],
                    samples[samples.size() * 95 / 100],
                    samples.back());
    EXPECT_EQ(m_physics.GetBodyCount(), 11u);
}
