#include <Framework/Entities/EntityWorldAsset.h>
#include <GameFramework/GraphicsSystems.h>
#include <gtest/gtest.h>

using namespace FE;
using namespace FE::Framework;
using namespace FE::GameFramework;

namespace
{
    struct TestView final : Graphics::View
    {
        TestView()
            : View(nullptr)
        {
        }


        void Update(Graphics::Core::FrameGraphBlackboard&) override {}


        void DoRelease() override
        {
            Memory::DefaultDelete(this);
        }
    };


    struct TestMeshes final : MeshSceneBridge
    {
        uint32_t m_created = 0, m_destroyed = 0, m_updates = 0;
        Matrix4x4 m_matrix = Matrix4x4::kIdentity;
        LifecycleResult Create(const MeshComponent&, MeshRuntimeComponent& runtime) override
        {
            runtime.m_handles.push_back({ ++m_created, 1 });
            return LifecycleResult::kSucceeded;
        }


        void Destroy(MeshRuntimeComponent& runtime) override
        {
            m_destroyed += runtime.m_handles.size();
            runtime.m_handles.clear();
        }


        void Update(const MeshComponent&, const Matrix4x4& matrix, MeshRuntimeComponent&) override
        {
            ++m_updates;
            m_matrix = matrix;
        }
    };


    struct TestScene final : Graphics::Scene
    {
        TestScene()
            : Scene(nullptr)
        {
        }


        Graphics::View* CreateView() override
        {
            m_views.push_back(Memory::DefaultNew<TestView>());
            return m_views.back().Get();
        }


        void DestroyView(Graphics::View* view) override
        {
            const auto found = festd::find_if(m_views.begin(), m_views.end(), [view](const Rc<Graphics::View>& candidate) {
                return candidate.Get() == view;
            });
            FE_Assert(found != m_views.end());
            m_views.erase(found);
        }


        uint32_t GetViewCount() const override
        {
            return m_views.size();
        }


        Graphics::View* GetView(uint32_t index) const override
        {
            return m_views[index].Get();
        }

    private:
        void DoRelease() override
        {
            Memory::DefaultDelete(this);
        }

        festd::vector<Rc<Graphics::View>> m_views;
    };


    struct TestSceneService final : WorldGraphicsSceneService
    {
    protected:
        Graphics::Scene* CreateScene() override
        {
            return Memory::DefaultNew<TestScene>();
        }


        MeshSceneBridge* CreateMeshBridge(Graphics::Scene&) override
        {
            return Memory::DefaultNew<TestMeshes>();
        }
    };


    struct GraphicsFixture : testing::Test
    {
        TestSceneService m_scene;
        EntityWorld m_world;
        TransformationSystem m_transforms;
        CameraSystem m_camera;
        MeshSystem m_mesh;
        Uuid m_meshUuid = Uuid::Random(), m_cameraUuid = Uuid::Random();

        TestMeshes& Meshes()
        {
            return static_cast<TestMeshes&>(m_scene.GetMeshes());
        }


        Graphics::View& CameraView()
        {
            return *m_world.Find(m_cameraUuid)->FindComponent<CameraRuntimeComponent>()->m_view;
        }


        void SetUp() override
        {
            m_world.AddService(m_scene);
            m_world.AddSystem(m_transforms);
            m_world.AddSystem(m_camera);
            m_world.AddSystem(m_mesh);
            auto& registry = m_world.CreateRegistry();
            EntityCommandList commands(m_world);
            const auto mesh = commands.CreateEntity(registry, {}, m_meshUuid);
            commands.AddComponent(mesh, TransformComponent{ Transform::Translation(Vector3(1, 2, 3)) });
            commands.AddComponent<MeshComponent>(mesh);
            const auto camera = commands.CreateEntity(registry, {}, m_cameraUuid);
            commands.AddComponent(camera, TransformComponent{ Transform::Translation(Vector3(0, 3, -8)) });
            commands.AddComponent<CameraComponent>(camera);
            m_world.Submit(std::move(commands));
            ASSERT_TRUE(m_world.CommitBootstrap());
        }


        void TearDown() override
        {
            m_world.Clear();
            m_world.RemoveSystem(m_mesh);
            m_world.RemoveSystem(m_camera);
            m_world.RemoveSystem(m_transforms);
            m_world.RemoveService(m_scene);
        }


        void Tick()
        {
            m_world.BeginUpdate();
            m_world.SchedulePhase(GameFramework::Phases::Transformation);
            m_world.SchedulePhase(GameFramework::Phases::GraphicsExtraction);
            ASSERT_TRUE(m_world.ExecuteSchedule());
            ASSERT_TRUE(m_world.EndUpdate());
        }
    };
} // namespace


TEST_F(GraphicsFixture, ExtractsChangedMatricesAndSkipsUnchangedChunks)
{
    Tick();
    EXPECT_EQ(Meshes().m_created, 1);
    EXPECT_FLOAT_EQ(Meshes().m_matrix.m_30, 1);
    EXPECT_FLOAT_EQ(CameraView().GetViewMatrix().m_32, 8);
    const auto updates = Meshes().m_updates;
    Tick();
    EXPECT_EQ(Meshes().m_updates, updates);
    EXPECT_EQ(m_world.GetScheduleDiagnostics().m_callbacks, 0);
    EntityCommandList commands(m_world);
    commands.ReplaceComponent(m_world.Find(m_meshUuid)->GetID(), TransformComponent{ Transform::Translation(Vector3(9, 0, 0)) });
    CameraComponent camera;
    camera.m_fovY = 0.5f;
    commands.ReplaceComponent(m_world.Find(m_cameraUuid)->GetID(), camera);
    commands.ReplaceComponent(m_world.Find(m_cameraUuid)->GetID(),
                              TransformComponent{ Transform::Translation(Vector3(0, 4, -12)) });
    m_world.Submit(std::move(commands));
    Tick();
    EXPECT_FLOAT_EQ(Meshes().m_matrix.m_30, 9);
    EXPECT_FLOAT_EQ(CameraView().GetViewMatrix().m_31, -4);
    EXPECT_FLOAT_EQ(CameraView().GetViewMatrix().m_32, 12);
    const auto projection = Matrix4x4::Projection(0.5f, 16.0f / 9.0f, 0.01f, 1000);
    EXPECT_FLOAT_EQ(CameraView().GetProjectionMatrix().m_11, projection.m_11);
}


TEST_F(GraphicsFixture, MembershipSurvivesMigrationAndIsExcludedFromSnapshots)
{
    Tick();
    const auto handle = m_world.Find(m_meshUuid)->FindComponent<const MeshRuntimeComponent>()->m_handles[0];
    EntityCommandList commands(m_world);
    commands.AddComponent(m_world.Find(m_meshUuid)->GetID(), NonUniformScaleComponent{ Vector3(2, 3, 4) });
    m_world.Submit(std::move(commands));
    Tick();
    EXPECT_EQ(m_world.Find(m_meshUuid)->FindComponent<const MeshRuntimeComponent>()->m_handles[0], handle);
    EXPECT_EQ(Meshes().m_destroyed, 0);
    EntityWorldSnapshotAsset snapshot;
    ASSERT_TRUE(m_world.CaptureSnapshot(snapshot));
    for (const auto& entity : snapshot.m_registries[0].m_entities.m_entities)
    {
        for (const auto& component : entity.m_components)
        {
            EXPECT_NE(component.m_type, Rtti::GetTypeID<MeshRuntimeComponent>());
            EXPECT_NE(component.m_type, Rtti::GetTypeID<CameraRuntimeComponent>());
            EXPECT_NE(component.m_type, Rtti::GetTypeID<WorldTransformComponent>());
        }
    }
}


TEST_F(GraphicsFixture, RemovingAuthoredMeshRemovesSceneMembershipAndCompanion)
{
    Tick();
    EntityCommandList commands(m_world);
    commands.RemoveComponent<MeshComponent>(m_world.Find(m_meshUuid)->GetID());
    m_world.Submit(std::move(commands));
    Tick();
    EXPECT_EQ(Meshes().m_destroyed, 1);
    EXPECT_EQ(m_world.Find(m_meshUuid)->FindComponent<const MeshRuntimeComponent>(), nullptr);
}


TEST_F(GraphicsFixture, CamerasCreateDistinctViewsAndRetainThemWhileInactive)
{
    Tick();
    ASSERT_EQ(m_scene.GetScene().GetViewCount(), 1);
    Rc<Graphics::View> original = &CameraView();
    const auto uuid = Uuid::Random();
    EntityCommandList commands(m_world);
    const auto entity = commands.CreateEntity(m_world.Find(m_cameraUuid)->GetRegistry(), {}, uuid);
    commands.AddComponent<TransformComponent>(entity);
    commands.AddComponent<CameraComponent>(entity);
    m_world.Submit(std::move(commands));
    Tick();
    ASSERT_EQ(m_scene.GetScene().GetViewCount(), 2);
    auto* other = m_world.Find(uuid)->FindComponent<CameraRuntimeComponent>();
    EXPECT_NE(other->m_view.Get(), original.Get());
    EXPECT_TRUE(other->m_view->IsEnabled());

    EntityCommandList deactivate(m_world);
    deactivate.SetActive(m_world.Find(m_cameraUuid)->GetID(), false);
    m_world.Submit(std::move(deactivate));
    Tick();
    EXPECT_FALSE(original->IsEnabled());
    EXPECT_EQ(m_scene.GetScene().GetViewCount(), 2);

    EntityCommandList activate(m_world);
    activate.SetActive(m_world.Find(m_cameraUuid, false)->GetID(), true);
    m_world.Submit(std::move(activate));
    Tick();
    EXPECT_EQ(&CameraView(), original.Get());
    EXPECT_TRUE(original->IsEnabled());
    EXPECT_EQ(m_scene.GetScene().GetViewCount(), 2);
}


TEST_F(GraphicsFixture, InvalidCameraContentLeavesPreviousMatricesUsable)
{
    Tick();
    const auto matrix = CameraView().GetProjectionMatrix();
    EntityCommandList commands(m_world);
    CameraComponent camera;
    camera.m_nearPlane = -1;
    commands.ReplaceComponent(m_world.Find(m_cameraUuid)->GetID(), camera);
    m_world.Submit(std::move(commands));
    Tick();
    EXPECT_FLOAT_EQ(CameraView().GetProjectionMatrix().m_22, matrix.m_22);
}


TEST_F(GraphicsFixture, ExplicitRuntimeRemovalAndWorldUnloadReleaseMembership)
{
    Tick();
    const auto updates = Meshes().m_updates;
    EntityCommandList commands(m_world);
    commands.RemoveComponent<MeshRuntimeComponent>(m_world.Find(m_meshUuid)->GetID());
    commands.RemoveComponent<CameraRuntimeComponent>(m_world.Find(m_cameraUuid)->GetID());
    m_world.Submit(std::move(commands));
    Tick();
    EXPECT_EQ(Meshes().m_destroyed, 1);
    EXPECT_EQ(m_scene.GetScene().GetViewCount(), 0);
    EXPECT_EQ(Meshes().m_updates, updates);

    m_world.Clear();
    EXPECT_EQ(Meshes().m_destroyed, Meshes().m_created);
    EXPECT_EQ(m_world.GetEntityCount(), 0);
}
