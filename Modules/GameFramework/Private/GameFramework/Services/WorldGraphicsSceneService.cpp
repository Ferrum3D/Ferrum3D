#include <GameFramework/Services/WorldGraphicsSceneService.h>
#include <Graphics/Assets/Streamers.h>
#include <Graphics/Passes/DeferredShadingPass.h>
#include <Graphics/Passes/DepthPyramidPass.h>
#include <Graphics/Passes/GBufferDebugPass.h>
#include <Graphics/Passes/SurfaceEvaluationPass.h>
#include <Graphics/Passes/TonemapPass.h>
#include <Graphics/Passes/VisibilityDebug.h>
#include <Graphics/Passes/VisibilityPass.h>
#include <Graphics/Renderer.h>
#include <Graphics/Scene/Culling.h>

namespace FE::GameFramework
{
    bool WorldGraphicsSceneService::Configure(const Configuration& settings)
    {
        FE_Assert(!m_scene, "Configure services before Init");
        return ApplyConfiguration(settings);
    }


    bool WorldGraphicsSceneService::ApplyConfiguration(const Configuration& settings)
    {
        if (!settings.Validate())
            return false;

        m_configuration = settings;
        if (m_scene)
        {
            if (auto* lighting = m_scene->GetModules().TryFind<Graphics::GlobalLightingSceneModule>())
                FE_Verify(lighting->SetParameters(settings));
        }

        return true;
    }


    WorldGraphicsSceneService::Configuration WorldGraphicsSceneService::CaptureConfiguration() const
    {
        return m_configuration;
    }


    void WorldGraphicsSceneService::Init(Framework::EntityWorld&)
    {
        FE_PROFILER_ZONE();

        FE_Assert(!m_scene);
        m_scene = CreateScene();
        FE_Verify(ApplyConfiguration(m_configuration));
        m_meshes.reset(CreateMeshBridge(*m_scene));
    }


    void WorldGraphicsSceneService::Shutdown(Framework::EntityWorld&)
    {
        FE_PROFILER_ZONE();

        FE_Assert(m_scene && m_scene->GetViewCount() == 0);
        m_meshes.reset();

        if (m_scene->GetModules().Contains<Graphics::MeshSceneModule>())
            m_scene->GetModules().Remove<Graphics::MeshSceneModule>();
        if (m_scene->GetModules().Contains<Graphics::GlobalLightingSceneModule>())
            m_scene->GetModules().Remove<Graphics::GlobalLightingSceneModule>();
        if (Graphics::Renderer* renderer = m_scene->GetRenderer())
            renderer->DestroyScene(m_scene.Get());

        m_scene.Reset();
    }


    Graphics::Scene& WorldGraphicsSceneService::GetScene() const
    {
        return *m_scene;
    }


    MeshSceneBridge& WorldGraphicsSceneService::GetMeshes() const
    {
        return *m_meshes;
    }


    Graphics::View* WorldGraphicsSceneService::CreateCameraView()
    {
        FE_PROFILER_ZONE();

        Graphics::View* view = GetScene().CreateView();
        view->GetModules().Add<Graphics::Culling::ViewModule>();
        view->GetModules().Add<Graphics::Visibility::ViewModule>();
        view->GetModules().Add<Graphics::DepthPyramidPass::ViewModule>();
        view->GetModules().Add<Graphics::SurfaceEvaluation::ViewModule>();
        view->GetModules().Add<Graphics::DeferredShading::ViewModule>();
        view->GetModules().Add<Graphics::Visibility::DebugViewModule>();
        view->GetModules().Add<Graphics::GBufferDebug::ViewModule>();
        view->GetModules().Add<Graphics::Tonemap::ViewModule>();
        view->SetEnabled(false);
        return view;
    }


    Graphics::Scene* WorldGraphicsSceneService::CreateScene()
    {
        FE_PROFILER_ZONE();

        Graphics::Scene* scene = Graphics::Renderer::Get().CreateScene();
        scene->GetModules().Add<Graphics::MeshSceneModule>();
        scene->GetModules().Add<Graphics::GlobalLightingSceneModule>();
        return scene;
    }


    MeshSceneBridge* WorldGraphicsSceneService::CreateMeshBridge(Graphics::Scene& scene)
    {
        FE_PROFILER_ZONE();

        auto* meshes = IO::AssetManager::FindStreamer(Rtti::GetTypeID<Graphics::MeshAsset>());
        auto* textures = IO::AssetManager::FindStreamer(Rtti::GetTypeID<Graphics::TextureAsset>());
        return Memory::DefaultNew<MeshSceneBridge>(&scene.GetModules().Find<Graphics::MeshSceneModule>(),
                                                   static_cast<Graphics::MeshStreamer*>(meshes),
                                                   static_cast<Graphics::TextureStreamer*>(textures));
    }
} // namespace FE::GameFramework
