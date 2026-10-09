#pragma once
#include <GameFramework/GraphicsComponents.h>

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
        //! @brief Bind borrowed scene/streaming services; optional streamers select initial mesh/texture detail.
        explicit MeshSceneBridge(Graphics::MeshSceneModule* module = nullptr, Graphics::MeshStreamer* meshes = nullptr,
                                 Graphics::TextureStreamer* textures = nullptr);
        //! @brief Destroy the bridge after all entity membership has been removed.
        virtual ~MeshSceneBridge() = default;
        //! @brief Create membership from ready logical assets; invalid content fails without asserting.
        virtual Framework::LifecycleResult Create(const MeshComponent& component, MeshRuntimeComponent& runtime);
        //! @brief Remove every instance and its batch before dependency release.
        virtual void Destroy(MeshRuntimeComponent& runtime);
        //! @brief Submit changed transform/material/model state; retains no component addresses.
        virtual void Update(const MeshComponent& component, const Matrix4x4& world, MeshRuntimeComponent& runtime);

    private:
        Graphics::MeshSceneModule* m_module;
        Graphics::MeshStreamer* m_meshes;
        Graphics::TextureStreamer* m_textures;
    };
} // namespace FE::GameFramework
