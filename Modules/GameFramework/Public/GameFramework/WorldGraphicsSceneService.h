#pragma once
#include <Framework/Entities/EntityWorld.h>
#include <GameFramework/MeshSceneBridge.h>

namespace FE::GameFramework
{
    //! @brief Own the world's graphics scene and mesh integration until all entity membership is removed.
    struct WorldGraphicsSceneService : Framework::WorldService
    {
        FE_RTTI("aaa69126-4427-4055-b1c4-04013aae10a5");

        //! @brief Create the world scene and its mesh module after renderer and asset streamer initialization.
        void Init(Framework::EntityWorld& world) override;
        //! @brief Remove the scene from the renderer after entities and their Views have been destroyed.
        void Shutdown(Framework::EntityWorld& world) override;
        //! @brief Borrow the initialized world scene.
        [[nodiscard]] Graphics::Scene& GetScene() const;
        //! @brief Borrow the initialized mesh integration bridge.
        [[nodiscard]] MeshSceneBridge& GetMeshes() const;
        //! @brief Create a disabled camera View with the world's depth and opaque passes.
        [[nodiscard]] Graphics::View* CreateCameraView();

    protected:
        //! @brief Create the scene and mesh module; integrations may override this to supply a different scene implementation.
        virtual Graphics::Scene* CreateScene();
        //! @brief Create scene integration using registered engine streamers.
        virtual MeshSceneBridge* CreateMeshBridge(Graphics::Scene& scene);

    private:
        Rc<Graphics::Scene> m_scene;
        festd::unique_ptr<MeshSceneBridge> m_meshes;
    };
} // namespace FE::GameFramework
