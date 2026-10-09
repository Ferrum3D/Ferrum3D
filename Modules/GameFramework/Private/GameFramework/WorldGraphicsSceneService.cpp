#include <GameFramework/WorldGraphicsSceneService.h>
#include <Graphics/Assets/Streamers.h>
#include <Graphics/Passes/DepthPrepass.h>
#include <Graphics/Passes/OpaquePass.h>
#include <Graphics/Renderer.h>

namespace FE::GameFramework
{
    void WorldGraphicsSceneService::Init(Framework::EntityWorld&)
    {
        FE_PROFILER_ZONE();
        FE_Assert(!m_scene);
        m_scene = CreateScene();
        m_meshes.reset(CreateMeshBridge(*m_scene));
    }


    void WorldGraphicsSceneService::Shutdown(Framework::EntityWorld&)
    {
        FE_PROFILER_ZONE();
        FE_Assert(m_scene && m_scene->GetViewCount() == 0);
        m_meshes.reset();
        if (m_scene->GetModules().Contains<Graphics::MeshSceneModule>())
            m_scene->GetModules().Remove<Graphics::MeshSceneModule>();
        if (Graphics::Renderer* renderer = m_scene->GetRenderer())
            renderer->DestroyScene(m_scene.Get());

        m_scene.Reset();
    }


    Graphics::Scene& WorldGraphicsSceneService::GetScene() const
    {
        FE_Assert(m_scene);
        return *m_scene;
    }


    MeshSceneBridge& WorldGraphicsSceneService::GetMeshes() const
    {
        FE_Assert(m_meshes);
        return *m_meshes;
    }


    Graphics::View* WorldGraphicsSceneService::CreateCameraView()
    {
        FE_PROFILER_ZONE();
        Graphics::View* view = GetScene().CreateView();
        view->GetModules().Add<Graphics::DepthPrepass::ViewModule>();
        view->GetModules().Add<Graphics::OpaquePass::ViewModule>();
        view->SetEnabled(false);
        return view;
    }


    Graphics::Scene* WorldGraphicsSceneService::CreateScene()
    {
        Graphics::Scene* scene = Graphics::Renderer::Get().CreateScene();
        scene->GetModules().Add<Graphics::MeshSceneModule>();
        return scene;
    }


    MeshSceneBridge* WorldGraphicsSceneService::CreateMeshBridge(Graphics::Scene& scene)
    {
        auto* meshes =
            static_cast<Graphics::MeshStreamer*>(IO::AssetManager::FindStreamer(Rtti::GetTypeID<Graphics::MeshAsset>()));
        auto* textures =
            static_cast<Graphics::TextureStreamer*>(IO::AssetManager::FindStreamer(Rtti::GetTypeID<Graphics::TextureAsset>()));
        return Memory::DefaultNew<MeshSceneBridge>(&scene.GetModules().Find<Graphics::MeshSceneModule>(), meshes, textures);
    }
} // namespace FE::GameFramework
