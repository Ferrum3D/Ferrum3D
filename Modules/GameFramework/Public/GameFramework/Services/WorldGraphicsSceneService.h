#pragma once
#include <Framework/Entities/EntityWorld.h>
#include <GameFramework/Graphics/MeshSceneBridge.h>

namespace FE::GameFramework
{
    //! @brief Own the world's graphics scene and mesh integration until all entity membership is removed.
    struct WorldGraphicsSceneService : Framework::WorldService
    {
        FE_RTTI("09F03B91-8406-485F-A7D1-A2063CA48020");

        void Init(Framework::EntityWorld& world) override;
        void Shutdown(Framework::EntityWorld& world) override;

        [[nodiscard]] Graphics::Scene& GetScene() const;
        [[nodiscard]] MeshSceneBridge& GetMeshes() const;

        //! @brief Create a disabled camera View with the deferred scene pipeline, its diagnostics and tonemapping.
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
