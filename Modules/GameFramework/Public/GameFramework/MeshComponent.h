#pragma once
#include <Framework/Entities/EntityComponentRegistry.h>
#include <Graphics/Features/Mesh/MeshSceneModule.h>

namespace FE::GameFramework
{
    struct WorldGraphicsSceneService;


    //! @brief Authored mesh assets; copying and serialization exclude scene membership.
    struct MeshComponent final
    {
        //! @brief Hard model dependency whose mesh products supply geometry.
        IO::Link<Graphics::ModelAsset> m_model;
        //! @brief Hard material dependency shared by this model's instances.
        IO::Link<Graphics::MaterialInstanceAsset> m_material;

        //! @brief Borrowed world scene service used to undo runtime membership.
        FE_SKIP_SERIALIZING WorldGraphicsSceneService* m_scene = nullptr;
        //! @brief Scene batch owned until deactivation.
        FE_SKIP_SERIALIZING Graphics::MeshBatch* m_batch = nullptr;
        //! @brief Generation-checked scene instances; relocation transfers ownership.
        FE_SKIP_SERIALIZING festd::vector<Graphics::MeshHandle> m_handles;
        //! @brief Last submitted model identity.
        FE_SKIP_SERIALIZING IO::AssetID m_loadedModel = IO::AssetID::kNull;
        //! @brief Last submitted material identity.
        FE_SKIP_SERIALIZING IO::AssetID m_loadedMaterial = IO::AssetID::kNull;

        MeshComponent() = default;
        MeshComponent(IO::Link<Graphics::ModelAsset> model, IO::Link<Graphics::MaterialInstanceAsset> material);
        MeshComponent(const MeshComponent& other);
        MeshComponent(MeshComponent&&) noexcept = default;

        Framework::LifecycleResult Activate(Framework::ComponentContext& context);
        void Deactivate(Framework::ComponentContext& context);

        FE_RTTI_Reflect("73077BD1-7103-43F9-B94A-FFCC1DCE71F5");
        FE_RTTI_Serialize();
    };
} // namespace FE::GameFramework
