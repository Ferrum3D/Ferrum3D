#include <Core/CLI/CommandLine.h>
#include <Core/IO/BaseIO.h>
#include <Core/Jobs/Jobs.h>
#include <GameFramework/Application.h>
#include <GameFramework/Components/MeshComponent.h>
#include <GameFramework/Editor/Application.h>
#include <GameFramework/Services/WorldGraphicsSceneService.h>
#include <GameSample/GameSample.h>
#include <Graphics/Scene/Scene.h>
#include <Graphics/Scene/View.h>
#include <imgui.h>

using namespace FE;

static const IO::AssetID kWorldAssetId("AAA69126-4427-4055-B1C4-04013AAE10A0");

namespace
{
    constexpr const char* kLitModeLabel = "lit";


    //! Shows the debug modes offered by the world's camera views and applies the selected one to all of them.
    struct RenderingControls final
    {
        //! Invalid for lit output; otherwise a mode name published by a view module.
        Env::Name m_debugMode;
        bool m_isDebugWindowDetached = false;
        bool m_isDemoWindowVisible = false;

        void Draw(Framework::EntityWorld& world, bool visible = true)
        {
            const auto* graphics = world.FindService<GameFramework::WorldGraphicsSceneService>();
            const Graphics::Scene* scene = graphics ? &graphics->GetScene() : nullptr;
            if (!visible)
            {
                if (scene)
                    ApplyDebugMode(*scene);

                return;
            }

            PlaceDebugWindow();
            if (ImGui::Begin("Rendering", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
                if (scene != nullptr)
                    DrawDebugModeSelector(*scene);
                else
                    ImGui::TextDisabled("The world has no graphics scene");

                ImGui::Checkbox("ImGui demo", &m_isDemoWindowVisible);
            }

            ImGui::End();

            if (m_isDemoWindowVisible)
                ImGui::ShowDemoWindow(&m_isDemoWindowVisible);

            if (scene != nullptr)
                ApplyDebugMode(*scene);
        }

    private:
        void PlaceDebugWindow() const
        {
            const ImGuiViewport* mainViewport = ImGui::GetMainViewport();
            if (!m_isDebugWindowDetached)
            {
                const ImVec2 position(mainViewport->WorkPos.x + 16.0f, mainViewport->WorkPos.y + 16.0f);
                ImGui::SetNextWindowPos(position, ImGuiCond_FirstUseEver);
                return;
            }

            // Outside the main viewport, ImGui moves the window into a platform window of its own.
            const ImVec2 position(mainViewport->Pos.x + mainViewport->Size.x + 32.0f, mainViewport->Pos.y);
            ImGui::SetNextWindowPos(position, ImGuiCond_Always);
        }


        void DrawDebugModeSelector(const Graphics::Scene& scene)
        {
            ImGui::Text("Camera views: %u", scene.GetViewCount());

            const char* preview = m_debugMode.IsValid() ? m_debugMode.c_str() : kLitModeLabel;
            if (!ImGui::BeginCombo("Debug mode", preview))
                return;

            if (ImGui::Selectable(kLitModeLabel, !m_debugMode.IsValid()))
                m_debugMode = {};

            // Modes are grouped by the module that implements them; views of one scene share their module set.
            if (scene.GetViewCount() > 0)
            {
                scene.GetView(0)->GetModules().ForEachActive([this](const Graphics::ViewModuleBase& module) {
                    const festd::span<const Env::Name> modes = module.GetDebugModes();
                    if (modes.empty())
                        return;

                    ImGui::Separator();
                    for (const Env::Name mode : modes)
                    {
                        if (ImGui::Selectable(mode.c_str(), m_debugMode == mode))
                            m_debugMode = mode;
                    }
                });
            }

            ImGui::EndCombo();
        }


        void ApplyDebugMode(const Graphics::Scene& scene) const
        {
            for (uint32_t viewIndex = 0; viewIndex < scene.GetViewCount(); ++viewIndex)
                scene.GetView(viewIndex)->SetDebugMode(m_debugMode);
        }
    };

    struct GameSampleApplication final : GameFramework::Application
    {
        RenderingControls m_controls;

    protected:
        void DrawImGuiWindows() override
        {
            m_controls.Draw(GetWorld());
        }
    };


    struct GameSampleEditorApplication final : Editor::Application
    {
        RenderingControls m_controls;
        bool m_smokePickRequested = false;
        bool m_renderingWindowVisible = false;

    protected:
        void DrawGameEditorMenu() override
        {
            if (ImGui::BeginMenu("Game"))
            {
                ImGui::MenuItem("Rendering controls", nullptr, &m_renderingWindowVisible);
                ImGui::EndMenu();
            }
        }

        void DrawGameEditorWindows() override
        {
            m_controls.Draw(GetWorld(), m_renderingWindowVisible);
            if (!Cli::Check("--smoke") || m_smokePickRequested)
                return;

            GetWorld().TraverseRoots([this](Framework::Entity& entity) {
                if (entity.IsActive() && entity.FindComponent<const GameFramework::MeshComponent>())
                {
                    Select(entity.GetID());
                    m_smokePickRequested = Pick(Vector2(0.5f, 0.5f));
                    return false;
                }

                return true;
            });
        }
    };
} // namespace


int FE::GameSample::Run(const int argc, const char** argv, const bool editor)
{
    Env::ApplicationInfo info;
    info.m_name = editor ? "Ferrum3D - Game Sample Editor" : "Ferrum3D - Game Sample";
    Env::Init(info, argc, argv);

    // The shader cache resolves engine sources relative to the sample build directory.
    IO::Directory::SetCurrentDirectory(FE_GAME_RUNTIME_DIRECTORY);

    GameFramework::Application* application = nullptr;
    RenderingControls* controls = nullptr;
    if (editor)
    {
        auto* instance = Memory::DefaultNew<GameSampleEditorApplication>();
        application = instance;
        controls = &instance->m_controls;
    }
    else
    {
        auto* instance = Memory::DefaultNew<GameSampleApplication>();
        application = instance;
        controls = &instance->m_controls;
    }

    controls->m_isDebugWindowDetached = Cli::Check("--detach-debug-window");

    // "deferred" names the lit pipeline in command lines predating per-module debug modes.
    if (const auto debugMode = Cli::GetValue("--visibility-debug"); debugMode && *debugMode != "deferred")
        controls->m_debugMode = Env::Name(*debugMode);

    GameFramework::ApplicationSettings settings;
    settings.m_assetDirectory = FE_GAME_ASSET_DIRECTORY;
    settings.m_world = kWorldAssetId;

    int32_t exitCode = 0;
    const bool smoke = Cli::Check("--smoke");
    Jobs::DispatchMainThread([&] {
        const bool initialized = application->Initialize(settings);
        if (initialized)
        {
            exitCode = application->Run(smoke ? 8 : 0);
            if (smoke && exitCode == 0)
            {
                const auto diagnostics = application->GetWorld().GetScheduleDiagnostics();
                Logger::LogInfo(
                    "GameSample rendered its disk world successfully: {} scheduler jobs, {} cascade batches, {} callbacks",
                    diagnostics.m_jobs,
                    diagnostics.m_treeBatches,
                    diagnostics.m_callbacks);
            }
        }
        else
        {
            exitCode = 1;
        }

        application->Shutdown();
        Jobs::StopJobSystem();
    });

    Jobs::StartJobSystem();
    Memory::DefaultDelete(application);
    Env::Shutdown();
    return exitCode;
}
