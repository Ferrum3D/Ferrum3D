#include "SampleAssets.h"
#include <AssetBuilder/AssetFile.h>
#include <AssetBuilder/AssetPipeline.h>
#include <Core/Jobs/Jobs.h>
#include <Framework/Entities/EntityWorldAsset.h>
#include <GameFramework/WorldStreamingService.h>

using namespace FE;

namespace
{
    bool AuthorWorld()
    {
        Framework::EntityWorldAsset world;
        world.m_systems = { Rtti::GetTypeID<GameFramework::TransformationSystem>(),
                            Rtti::GetTypeID<GameFramework::CameraSystem>(),
                            Rtti::GetTypeID<GameFramework::MeshSystem>() };
        world.m_services = { Rtti::GetTypeID<GameFramework::WorldGraphicsSceneService>(),
                             Rtti::GetTypeID<GameFramework::WorldStreamingService>() };
        auto& group = world.m_registries.emplace_back();
        group.m_key = Uuid("aaa69126-4427-4055-b1c4-04013aae10b0");
        Framework::EntityRecord helmet;
        helmet.m_uuid = Uuid("aaa69126-4427-4055-b1c4-04013aae10b1");
        helmet.m_name = "Helmet";
        const auto mesh = GameFramework::MeshComponent{
            IO::Link<Graphics::ModelAsset>(Uuid("968E3679-1025-43A2-AB3E-967494F90D35")),
            IO::Link<Graphics::MaterialInstanceAsset>(Uuid("44B73EB3-0933-4221-96C8-C9E09CBF36FD"))
        };
        const auto transform = Transform::Create(Vector3(0, 1.25f, 0), Quaternion::RotationY(Constants::kPI), 1.0f);
        if (!group.m_entities.CookComponent(helmet, mesh)
            || !group.m_entities.CookComponent(helmet, GameFramework::TransformComponent{ transform }))
        {
            return false;
        }

        group.m_entities.m_entities.push_back(std::move(helmet));
        Framework::EntityRecord camera;
        camera.m_uuid = Uuid("aaa69126-4427-4055-b1c4-04013aae10b2");
        camera.m_name = "Camera";
        const Vector3 position(0, 3, -8);
        const auto matrix = Math::Invert(Matrix4x4::LookAt(position, Vector3(0, 0.75f, 0), Vector3::AxisY()));
        const auto cameraTransform = Transform::Create(position, Math::ExtractRotation(matrix), 1.0f);
        if (!group.m_entities.CookComponent(camera, GameFramework::CameraComponent{})
            || !group.m_entities.CookComponent(camera, GameFramework::TransformComponent{ cameraTransform }))
        {
            return false;
        }

        group.m_entities.m_entities.push_back(std::move(camera));
        AssetBuilder::AssetFile file;
        auto& artifact = file.m_artifacts.emplace_back();
        artifact.m_productKey = "Serialized/Primary";
        artifact.m_name = "Helmet world";
        artifact.m_assetId = GameSample::kWorld;
        artifact.m_assetTypeId = Rtti::GetTypeID<Framework::EntityWorldAsset>();
        artifact.m_buildSettings.Emplace<Framework::EntityWorldAsset>(std::move(world));
        return AssetBuilder::SaveAssetFile(IO::Path(FE_GAME_SAMPLE_WORLD_SOURCE), file);
    }
} // namespace

int main(int argc, const char** argv)
{
    Env::ApplicationInfo info;
    info.m_name = "GameSampleAssetBuilder";
    Env::Init(info, argc, argv);
    int exitCode = 0;
    Jobs::DispatchMainThread([&] {
        const bool author = argc > 1 && strcmp(argv[1], "--author") == 0;
        if ((author && !AuthorWorld())
            || !AssetBuilder::BuildAsset({ IO::Path(FE_GAME_SAMPLE_WORLD_SOURCE), {}, IO::Path(FE_GAME_SAMPLE_ASSET_DIR) }))
        {
            exitCode = 1;
        }

        Jobs::StopJobSystem();
    });
    Jobs::StartJobSystem();
    Env::Shutdown();
    return exitCode;
}
