#include <AssetBuilder/AssetFile.h>
#include <AssetBuilder/AssetPipeline.h>
#include <Core/IO/Artifact.h>
#include <Core/IO/AssetManager.h>
#include <Core/IO/MemoryStream.h>
#include <Core/Serialization/BinarySerialization.h>
#include <Core/Threading/Thread.h>
#include <Framework/Entities/EntityWorldAsset.h>
#include <Framework/Entities/WorldTimeService.h>
#include <GameFramework/Components/CameraComponent.h>
#include <GameFramework/Components/CameraMovementComponent.h>
#include <GameFramework/Components/MeshComponent.h>
#include <GameFramework/Services/WorldGraphicsSceneService.h>
#include <GameFramework/Systems/CameraMovementSystem.h>
#include <GameFramework/Systems/CameraSystem.h>
#include <GameFramework/Systems/MeshSystem.h>
#include <GameFramework/Systems/TransformationSystem.h>
#include <gtest/gtest.h>

using namespace FE;
using namespace FE::Framework;
using namespace FE::GameFramework;

namespace
{
    template<class T>
    IO::Link<T> BuildCameraInputAsset(const IO::Path& directory, const char* name, const T& value)
    {
        const IO::Path path = directory / name;
        EXPECT_TRUE(AssetBuilder::ImportAsset({ {}, path, Rtti::GetTypeID<T>() }));
        AssetBuilder::AssetFile file;
        EXPECT_TRUE(AssetBuilder::LoadAssetFile(path, file));
        file.m_artifacts.front().m_buildSettings.Emplace<T>(value);
        const auto id = file.m_artifacts.front().m_assetId;
        EXPECT_TRUE(AssetBuilder::SaveAssetFile(path, file));
        EXPECT_TRUE(AssetBuilder::ImportAsset({ {}, path, Rtti::GetTypeID<T>() }));
        EXPECT_TRUE(AssetBuilder::BuildAsset({ path, {}, directory }));
        return IO::Link<T>(id);
    }


    struct CameraInputBackend final : Input::Core::Backend
    {
        Input::Core::KeyboardState m_keyboard;
        Input::Core::MouseState m_mouse;
        uint32_t m_updates = 0;
        festd::span<const Input::Core::DeviceKind> EnumerateDevices() const override
        {
            return {};
        }
        void Update() override
        {
            ++m_updates;
        }
        const Input::Core::KeyboardState& GetKeyboardState() const override
        {
            return m_keyboard;
        }
        const Input::Core::MouseState& GetMouseState() const override
        {
            return m_mouse;
        }
    };


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
        uint32_t m_created = 0, m_destroyed = 0;
        std::atomic<uint32_t> m_updates = 0;
        Matrix4x4 m_matrices[1024];
        Matrix4x4 m_matrix = Matrix4x4::kIdentity;
        LifecycleResult Create(MeshComponent& component) override
        {
            component.m_handles.push_back({ ++m_created, 1 });
            return LifecycleResult::kSucceeded;
        }


        void Destroy(MeshComponent& component) override
        {
            m_destroyed += component.m_handles.size();
            component.m_handles.clear();
        }


        void UpdateModel(MeshComponent&) override
        {
            EXPECT_TRUE(Threading::IsMainThread());
        }


        void UpdateTransform(const MeshComponent& component, const Matrix4x4& matrix) override
        {
            ++m_updates;
            const uint32_t handle = component.m_handles.front().m_value;
            m_matrices[handle] = matrix;
            if (handle == 1)
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
            return *m_world.Find(m_cameraUuid)->FindComponent<const CameraComponent>()->m_view;
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
    const auto updates = Meshes().m_updates.load();
    Tick();
    EXPECT_EQ(Meshes().m_updates.load(), updates);
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


TEST_F(GraphicsFixture, AuthoredInputMovesEveryCameraBeforeTransformAndGraphicsExtraction)
{
    const IO::Path directory = IO::Path(FE_GAME_ASSET_TEST_OUTPUT) / Fmt::Format("CameraInput-{}", Uuid::Random());
    Input::Action move;
    move.m_type = Input::ActionType::kVector2;
    move.m_processor = Memory::DefaultNew<Input::ContinuousActionProcessor>();
    const auto moveLink = BuildCameraInputAsset(directory, "move.asset", move);
    Input::Action elevate;
    elevate.m_type = Input::ActionType::kScalar;
    elevate.m_processor = Memory::DefaultNew<Input::ContinuousActionProcessor>();
    const auto elevateLink = BuildCameraInputAsset(directory, "elevate.asset", elevate);
    Input::MappingTable table;
    Input::ActionMapping mapping;
    mapping.m_action = moveLink;
    mapping.m_bindings = { Input::Binding::WASD() };
    table.m_mappings.push_back(mapping);
    mapping.m_action = elevateLink;
    Input::Binding elevateBinding;
    elevateBinding.m_source =
        Input::Axis1DBinding{ Input::Control::Key(Input::Core::Key::kQ), Input::Control::Key(Input::Core::Key::kE) };
    mapping.m_bindings = { elevateBinding };
    table.m_mappings.push_back(mapping);
    const auto tableLink = BuildCameraInputAsset(directory, "table.asset", table);
    IO::ArtifactStore::SetCatalogSource(directory);
    IO::AssetManager::Init();
    Framework::WorldTimeService time;
    Input::WorldInputService input;
    CameraMovementSystem movementSystem;
    auto* backend = Memory::DefaultNew<CameraInputBackend>();
    backend->m_keyboard.m_keyDownMap.set(87).set(69);
    input.SetBackend(festd::unique_ptr<Input::Core::Backend>(backend));
    m_world.AddService(time);
    m_world.AddService(input);
    m_world.AddSystem(movementSystem);
    auto cleanup = festd::defer([&] {
        m_world.Clear();
        m_world.RemoveSystem(movementSystem);
        m_world.RemoveService(input);
        m_world.RemoveService(time);
        IO::AssetManager::Shutdown();
    });

    auto request = IO::AssetManager::LoadAsset(tableLink);
    for (uint32_t iteration = 0; iteration < 5000 && !request.IsCompleted(); ++iteration)
    {
        IO::AssetManager::Tick();
        Threading::Sleep(1);
    }

    ASSERT_EQ(request.GetResult(), IO::AssetLoadResult::kSucceeded);
    CameraMovementComponent movement;
    (void)input.Activate(tableLink);
    movement.m_moveAction = moveLink;
    movement.m_elevateAction = elevateLink;
    EntityCommandList commands(m_world);
    commands.AddComponent(m_world.Find(m_cameraUuid)->GetID(), movement);
    const Uuid secondUuid = Uuid::Random();
    const auto second = commands.CreateEntity(m_world.Find(m_cameraUuid)->GetRegistry(), {}, secondUuid);
    commands.AddComponent<TransformComponent>(second);
    commands.AddComponent<CameraComponent>(second);
    commands.AddComponent(second, movement);
    m_world.Submit(std::move(commands));
    for (uint32_t iteration = 0; iteration < 5000; ++iteration)
    {
        IO::AssetManager::Tick();
        ASSERT_TRUE(m_world.CommitBootstrap());
        const auto* firstEntity = m_world.Find(m_cameraUuid);
        if (firstEntity && firstEntity->FindComponent<const CameraMovementComponent>() && m_world.Find(secondUuid))
            break;

        Threading::Sleep(1);
    }

    ASSERT_NE(m_world.Find(m_cameraUuid), nullptr);
    ASSERT_NE(m_world.Find(secondUuid), nullptr);
    m_world.BeginUpdate();
    ASSERT_TRUE(m_world.SchedulePhase(GameFramework::Phases::Input));
    ASSERT_TRUE(m_world.SchedulePhase(GameFramework::Phases::Transformation));
    ASSERT_TRUE(m_world.SchedulePhase(GameFramework::Phases::GraphicsExtraction));
    ASSERT_TRUE(m_world.ExecuteSchedule());
    ASSERT_TRUE(m_world.EndUpdate());
    EXPECT_EQ(backend->m_updates, 1);
    const float distance = movement.m_moveSpeed * time.GetDeltaSeconds();
    const auto* firstTransform = m_world.Find(m_cameraUuid)->FindComponent<const TransformComponent>();
    const auto* secondTransform = m_world.Find(secondUuid)->FindComponent<const TransformComponent>();
    ASSERT_NE(firstTransform, nullptr);
    ASSERT_NE(secondTransform, nullptr);
    EXPECT_FLOAT_EQ(firstTransform->m_local.Translation().z, -8 + distance);
    EXPECT_FLOAT_EQ(firstTransform->m_local.Translation().y, 3 + distance);
    EXPECT_FLOAT_EQ(secondTransform->m_local.Translation().z, distance);
    EXPECT_FLOAT_EQ(secondTransform->m_local.Translation().y, distance);
    EXPECT_FLOAT_EQ(CameraView().GetViewMatrix().m_32, 8 - distance);
    EXPECT_FLOAT_EQ(CameraView().GetViewMatrix().m_31, -3 - distance);
    backend->m_keyboard.m_keyDownMap.reset();
    const Matrix4x4 view = CameraView().GetViewMatrix();
    m_world.BeginUpdate();
    ASSERT_TRUE(m_world.SchedulePhase(GameFramework::Phases::Input));
    ASSERT_TRUE(m_world.SchedulePhase(GameFramework::Phases::Transformation));
    ASSERT_TRUE(m_world.SchedulePhase(GameFramework::Phases::GraphicsExtraction));
    ASSERT_TRUE(m_world.ExecuteSchedule());
    ASSERT_TRUE(m_world.EndUpdate());
    EXPECT_EQ(backend->m_updates, 2);
    EXPECT_EQ(m_world.GetScheduleDiagnostics().m_callbacks, 2);
    EXPECT_TRUE(Math::CmpEqual(CameraView().GetViewMatrix(), view));
}


TEST_F(GraphicsFixture, MembershipSurvivesMigrationAndIsExcludedFromSnapshots)
{
    Tick();
    const auto handle = m_world.Find(m_meshUuid)->FindComponent<const MeshComponent>()->m_handles[0];
    EntityCommandList commands(m_world);
    commands.AddComponent(m_world.Find(m_meshUuid)->GetID(), NonUniformScaleComponent{ Vector3(2, 3, 4) });
    m_world.Submit(std::move(commands));
    Tick();
    EXPECT_EQ(m_world.Find(m_meshUuid)->FindComponent<const MeshComponent>()->m_handles[0], handle);
    EXPECT_EQ(Meshes().m_destroyed, 0);
    EntityWorldSnapshotAsset snapshot;
    ASSERT_TRUE(m_world.CaptureSnapshot(snapshot));
    for (const auto& entity : snapshot.m_registries[0].m_entities.m_entities)
    {
        for (const auto& component : entity.m_components)
        {
            EXPECT_NE(component.GetType()->m_id, Rtti::GetTypeID<WorldTransformComponent>());
        }
    }
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
    auto* other = m_world.Find(uuid)->FindComponent<CameraComponent>();
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


TEST_F(GraphicsFixture, ComponentRemovalAndWorldUnloadReleaseMembership)
{
    Tick();
    const auto updates = Meshes().m_updates.load();
    EntityCommandList commands(m_world);
    commands.RemoveComponent<MeshComponent>(m_world.Find(m_meshUuid)->GetID());
    commands.RemoveComponent<CameraComponent>(m_world.Find(m_cameraUuid)->GetID());
    m_world.Submit(std::move(commands));
    Tick();
    EXPECT_EQ(Meshes().m_destroyed, 1);
    EXPECT_EQ(m_scene.GetScene().GetViewCount(), 0);
    EXPECT_EQ(Meshes().m_updates.load(), updates);

    m_world.Clear();
    EXPECT_EQ(Meshes().m_destroyed, Meshes().m_created);
    EXPECT_EQ(m_world.GetEntityCount(), 0);
}


TEST_F(GraphicsFixture, CopiesAndSerializationExcludeRuntimeOwnership)
{
    Tick();
    const auto& mesh = *m_world.Find(m_meshUuid)->FindComponent<const MeshComponent>();
    const auto& camera = *m_world.Find(m_cameraUuid)->FindComponent<const CameraComponent>();
    MeshComponent meshCopy(mesh);
    CameraComponent cameraCopy(camera);
    EXPECT_EQ(meshCopy.m_batch, nullptr);
    EXPECT_TRUE(meshCopy.m_handles.empty());
    EXPECT_EQ(cameraCopy.m_scene, nullptr);
    EXPECT_FALSE(cameraCopy.m_view);

    IO::WriteOnlyMemoryStream stream;
    Serialization::TaggedBinaryFormat format;
    Serialization::SerializationContext writer(&stream, format);
    ASSERT_EQ(writer.Store(mesh), Serialization::ResultCode::kSuccess);
    ASSERT_EQ(writer.Store(camera), Serialization::ResultCode::kSuccess);
    festd::pmr::vector<std::byte> bytes;
    stream.DumpAll(bytes);

    IO::ReadOnlyMemoryStream input(bytes);
    Serialization::DeserializationContext reader(&input, format);
    MeshComponent restoredMesh;
    CameraComponent restoredCamera;
    ASSERT_EQ(reader.Load(restoredMesh), Serialization::ResultCode::kSuccess);
    ASSERT_EQ(reader.Load(restoredCamera), Serialization::ResultCode::kSuccess);
    EXPECT_EQ(restoredMesh.m_model.GetAssetID(), mesh.m_model.GetAssetID());
    EXPECT_EQ(restoredMesh.m_material.GetAssetID(), mesh.m_material.GetAssetID());
    EXPECT_EQ(restoredMesh.m_batch, nullptr);
    EXPECT_TRUE(restoredMesh.m_handles.empty());
    EXPECT_EQ(restoredCamera.m_scene, nullptr);
    EXPECT_FALSE(restoredCamera.m_view);
    EXPECT_EQ(restoredCamera.m_fovY, camera.m_fovY);
}


TEST_F(GraphicsFixture, SnapshotRestoreRecreatesGraphicsOwnership)
{
    Tick();
    Rc<Graphics::View> previous = &CameraView();
    EntityWorldSnapshotAsset snapshot;
    ASSERT_TRUE(m_world.CaptureSnapshot(snapshot));
    m_world.Clear();
    EXPECT_EQ(m_scene.GetScene().GetViewCount(), 0);
    EXPECT_EQ(Meshes().m_destroyed, 1);

    ASSERT_TRUE(m_world.RestoreSnapshot(snapshot));
    // Bootstrap is closed after the first tick, so restoration advances at the next normal safe point.
    Tick();
    EXPECT_EQ(m_scene.GetScene().GetViewCount(), 1);
    EXPECT_NE(&CameraView(), previous.Get());
    EXPECT_EQ(Meshes().m_created, 2);
    EXPECT_TRUE(CameraView().IsEnabled());
}


TEST_F(GraphicsFixture, MeshReplacementReleasesPreviousMembership)
{
    Tick();
    Entity& entity = *m_world.Find(m_meshUuid);
    const auto& mesh = *entity.FindComponent<const MeshComponent>();
    const auto previous = mesh.m_handles.front();
    MeshComponent replacement(mesh);
    EntityCommandList commands(m_world);
    commands.ReplaceComponent(entity.GetID(), std::move(replacement));
    m_world.Submit(std::move(commands));
    Tick();
    EXPECT_EQ(Meshes().m_created, 2);
    EXPECT_EQ(Meshes().m_destroyed, 1);
    EXPECT_NE(entity.FindComponent<const MeshComponent>()->m_handles.front(), previous);
}


TEST_F(GraphicsFixture, ExtractsHundredsOfTransformsAfterMainThreadMembership)
{
    auto& registry = m_world.CreateRegistry();
    EntityCommandList commands(m_world);
    for (uint32_t i = 0; i < 500; ++i)
    {
        const auto entity = commands.CreateEntity(registry);
        commands.AddComponent(entity, TransformComponent{ Transform::Translation(Vector3(float(i), 7.0f, 13.0f)) });
        commands.AddComponent<MeshComponent>(entity);
    }

    m_world.Submit(std::move(commands));
    for (uint32_t frame = 0; frame < 20 && Meshes().m_created < 501; ++frame)
    {
        const uint32_t previousCount = Meshes().m_created;
        Tick();
        EXPECT_LE(Meshes().m_created - previousCount, 32);
    }

    ASSERT_EQ(Meshes().m_created, 501);
    EXPECT_GT(m_world.GetScheduleDiagnostics().m_jobs, 4);
    for (uint32_t i = 0; i < 500; ++i)
    {
        EXPECT_FLOAT_EQ(Meshes().m_matrices[i + 2].m_30, float(i));
        EXPECT_FLOAT_EQ(Meshes().m_matrices[i + 2].m_31, 7.0f);
        EXPECT_FLOAT_EQ(Meshes().m_matrices[i + 2].m_32, 13.0f);
    }

    const uint32_t updates = Meshes().m_updates.load();
    Tick();
    EXPECT_EQ(Meshes().m_updates.load(), updates);
}
