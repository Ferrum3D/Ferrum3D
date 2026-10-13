#pragma once
#include <GameFramework/Components/MeshComponent.h>

namespace FE::Graphics
{
    struct MeshStreamer;
    struct TextureStreamer;
} // namespace FE::Graphics

namespace FE::GameFramework
{
    //! @brief Scene integration seam; default implementation uses MeshSceneModule, tests may override it without a GPU.
    struct MeshSceneBridge
    {
        explicit MeshSceneBridge(Graphics::MeshSceneModule* module = nullptr, Graphics::MeshStreamer* meshes = nullptr,
                                 Graphics::TextureStreamer* textures = nullptr);
        virtual ~MeshSceneBridge() = default;

        //! @brief Create membership from ready logical assets; invalid content fails without asserting.
        virtual Framework::LifecycleResult Create(MeshComponent& component);
        //! @brief Remove every instance and its batch before dependency release.
        virtual void Destroy(MeshComponent& component);
        //! @brief Submit changed transform/material/model state; retains no component addresses.
        virtual void Update(MeshComponent& component, const Matrix4x4& world);

    private:
        Graphics::MeshSceneModule* m_module;
        Graphics::MeshStreamer* m_meshes;
        Graphics::TextureStreamer* m_textures;
    };
} // namespace FE::GameFramework
