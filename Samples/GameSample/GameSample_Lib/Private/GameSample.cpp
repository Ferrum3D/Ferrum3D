#include <Core/CLI/CommandLine.h>
#include <Core/IO/BaseIO.h>
#include <Core/Jobs/Jobs.h>
#include <GameFramework/Application.h>
#include <GameFramework/Components/MeshComponent.h>
#include <GameFramework/Editor/Application.h>
#include <GameFramework/Services/WorldGraphicsSceneService.h>
#include <GameSample/GameSample.h>
#include <Graphics/Scene/Culling.h>
#include <Graphics/Scene/Scene.h>
#include <Graphics/Scene/View.h>
#include <imgui.h>

using namespace FE;

static const IO::AssetID kWorldAssetId("AAA69126-4427-4055-B1C4-04013AAE10A0");

namespace
{
    constexpr const char* kLitModeLabel = "lit";
    constexpr const char* kLodModeLabel = "lod";

    constexpr ImU32 kFrozenFrustumColor = IM_COL32(255, 200, 40, 255);
    constexpr float kFrustumLineThickness = 2.0f;


    //! Projects the frozen culling frustum into the current camera, clipping its edges against the camera plane.
    struct FrustumOverlay final
    {
        Matrix4x4 m_viewProjection;
        ImVec2 m_origin;
        ImVec2 m_size;


        void Draw(ImDrawList& drawList, const Matrix4x4& frozenViewProjection) const
        {
            // Reverse-Z puts the near plane at depth one and the far plane at depth zero.
            const Matrix4x4 clipToWorld = Math::Invert(frozenViewProjection);
            Vector4 corners[8];
            for (uint32_t corner = 0; corner < 8; ++corner)
            {
                const Vector4 ndc((corner & 1) ? 1.0f : -1.0f, (corner & 2) ? 1.0f : -1.0f, (corner & 4) ? 1.0f : 0.0f, 1.0f);
                const Vector4 world = ndc * clipToWorld;
                corners[corner] = Vector4(world.x / world.w, world.y / world.w, world.z / world.w, 1.0f) * m_viewProjection;
            }

            constexpr uint32_t kEdges[12][2] = { { 0, 1 }, { 2, 3 }, { 4, 5 }, { 6, 7 }, { 0, 2 }, { 1, 3 },
                                                 { 4, 6 }, { 5, 7 }, { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 } };
            for (const auto& edge : kEdges)
                DrawEdge(drawList, corners[edge[0]], corners[edge[1]]);
        }

    private:
        static constexpr float kMinClipW = 1e-3f;


        [[nodiscard]] ImVec2 ToScreen(const Vector4& clip) const
        {
            const float x = (clip.x / clip.w) * 0.5f + 0.5f;
            const float y = 0.5f - (clip.y / clip.w) * 0.5f;
            return ImVec2(m_origin.x + x * m_size.x, m_origin.y + y * m_size.y);
        }


        void DrawEdge(ImDrawList& drawList, Vector4 start, Vector4 end) const
        {
            if (start.w < kMinClipW && end.w < kMinClipW)
                return;

            // Move the endpoint behind the camera onto the plane in front of it.
            if (start.w < kMinClipW)
                start = start + (end - start) * ((kMinClipW - start.w) / (end.w - start.w));
            else if (end.w < kMinClipW)
                end = end + (start - end) * ((kMinClipW - end.w) / (start.w - end.w));

            drawList.AddLine(ToScreen(start), ToScreen(end), kFrozenFrustumColor, kFrustumLineThickness);
        }
    };


    //! Shows the debug modes offered by the world's camera views and applies the selected one to all of them.
    struct RenderingControls final
    {
        //! Invalid for lit output; otherwise a mode name published by a view module.
        Env::Name m_debugMode;
        //! Applied to every camera view; statistics are shown for the first one.
        Graphics::Culling::Settings m_cullingSettings;
        bool m_isDebugWindowDetached = false;
        bool m_isDemoWindowVisible = false;

        void Draw(Framework::EntityWorld& world, bool visible = true)
        {
            const auto* graphics = world.FindService<GameFramework::WorldGraphicsSceneService>();
            const Graphics::Scene* scene = graphics ? &graphics->GetScene() : nullptr;
            if (!visible)
            {
                if (scene)
                {
                    ApplyDebugMode(*scene);
                    ApplyCullingSettings(*scene);
                }

                return;
            }

            PlaceDebugWindow();
            if (ImGui::Begin("Rendering", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
                if (scene != nullptr)
                {
                    DrawDebugModeSelector(*scene);
                    DrawCullingSettings();
                    DrawCullingStats(*scene);
                }
                else
                {
                    ImGui::TextDisabled("The world has no graphics scene");
                }

                ImGui::Checkbox("ImGui demo", &m_isDemoWindowVisible);
            }

            ImGui::End();

            if (m_isDemoWindowVisible)
                ImGui::ShowDemoWindow(&m_isDemoWindowVisible);

            if (scene != nullptr)
            {
                ApplyDebugMode(*scene);
                ApplyCullingSettings(*scene);
                DrawFrozenFrustum(*scene);
            }
        }


        //! Report the most recent statistics of the first camera view, e.g. at the end of a smoke run.
        static void LogCullingStats(Framework::EntityWorld& world)
        {
            const auto* graphics = world.FindService<GameFramework::WorldGraphicsSceneService>();
            if (graphics == nullptr || graphics->GetScene().GetViewCount() == 0)
                return;

            const Graphics::View& view = *graphics->GetScene().GetView(0);
            const auto* culling = view.GetModules().TryFind<Graphics::Culling::ViewModule>();
            if (culling == nullptr || !culling->GetStats().m_isValid)
            {
                Logger::LogWarning("No culling statistics were read back");
                return;
            }

            const Graphics::Culling::Stats& stats = culling->GetStats();
            Logger::LogInfo("Culling: {} instances tested, {} frustum culled, {} occlusion culled, {} rendered ({} late)",
                            stats.m_instanceCandidates,
                            stats.m_instanceFrustumCulled,
                            stats.m_instanceOcclusionCulled,
                            stats.m_instancesRendered,
                            stats.m_instancesRenderedLate);
            Logger::LogInfo("Culling: {} meshlets tested, {} frustum culled, {} occlusion culled, {} rendered, {} triangles",
                            stats.m_meshletCandidates,
                            stats.m_meshletFrustumCulled,
                            stats.m_meshletOcclusionCulled,
                            stats.m_meshletsRendered,
                            stats.m_trianglesRendered);
            for (uint32_t lodIndex = 0; lodIndex < Graphics::Culling::Stats::kMaxLodCount; ++lodIndex)
            {
                if (stats.m_lodInstances[lodIndex] != 0)
                    Logger::LogInfo("Culling: LOD {} rendered {} instances", lodIndex, stats.m_lodInstances[lodIndex]);
            }
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

            // Names are interned after the environment is initialized.
            const Env::Name lodMode(kLodModeLabel);
            bool isLodColored = m_debugMode == lodMode;
            if (ImGui::Checkbox("Color meshes by LOD", &isLodColored))
                m_debugMode = isLodColored ? lodMode : Env::Name{};

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


        void DrawCullingSettings()
        {
            if (!ImGui::CollapsingHeader("Culling", ImGuiTreeNodeFlags_DefaultOpen))
                return;

            Graphics::Culling::Settings& settings = m_cullingSettings;
            ImGui::Checkbox("Instance frustum culling", &settings.m_instanceFrustumCulling);
            ImGui::Checkbox("Instance occlusion culling", &settings.m_instanceOcclusionCulling);
            ImGui::Checkbox("Meshlet frustum culling", &settings.m_meshletFrustumCulling);
            ImGui::Checkbox("Meshlet occlusion culling", &settings.m_meshletOcclusionCulling);
            ImGui::Checkbox("LOD selection", &settings.m_lodSelection);

            ImGui::BeginDisabled(!settings.m_lodSelection);
            ImGui::SliderFloat("LOD error (px)",
                               &settings.m_lodErrorThreshold,
                               0.25f,
                               32.0f,
                               "%.2f",
                               ImGuiSliderFlags_Logarithmic);
            ImGui::EndDisabled();

            ImGui::Checkbox("Freeze culling camera", &settings.m_freezeCulling);
            ImGui::SetItemTooltip("All culling keeps the camera of the frame the freeze started.\n"
                                  "Occlusion culling tests against the depth pyramid captured in that frame.");
        }


        static void DrawStatRow(const char* label, const uint32_t value, const uint32_t total)
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(label);
            ImGui::TableNextColumn();
            ImGui::Text("%u", value);
            ImGui::TableNextColumn();
            if (total != 0)
                ImGui::Text("%.1f%%", 100.0f * static_cast<float>(value) / static_cast<float>(total));
        }


        static void DrawCullingStats(const Graphics::Scene& scene)
        {
            if (!ImGui::CollapsingHeader("Culling statistics", ImGuiTreeNodeFlags_DefaultOpen))
                return;

            const auto* culling =
                scene.GetViewCount() > 0 ? scene.GetView(0)->GetModules().TryFind<Graphics::Culling::ViewModule>() : nullptr;
            if (culling == nullptr || !culling->GetStats().m_isValid)
            {
                ImGui::TextDisabled("No statistics yet");
                return;
            }

            const Graphics::Culling::Stats& stats = culling->GetStats();
            if (ImGui::BeginTable("CullingStats", 3, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg))
            {
                DrawStatRow("Instances tested", stats.m_instanceCandidates, stats.m_instanceCandidates);
                DrawStatRow("  frustum culled", stats.m_instanceFrustumCulled, stats.m_instanceCandidates);
                DrawStatRow("  occlusion culled", stats.m_instanceOcclusionCulled, stats.m_instanceCandidates);
                DrawStatRow("  rendered", stats.m_instancesRendered, stats.m_instanceCandidates);
                DrawStatRow("    in late phase", stats.m_instancesRenderedLate, stats.m_instancesRendered);
                DrawStatRow("Meshlets tested", stats.m_meshletCandidates, stats.m_meshletCandidates);
                DrawStatRow("  frustum culled", stats.m_meshletFrustumCulled, stats.m_meshletCandidates);
                DrawStatRow("  occlusion culled", stats.m_meshletOcclusionCulled, stats.m_meshletCandidates);
                DrawStatRow("  rendered", stats.m_meshletsRendered, stats.m_meshletCandidates);
                DrawStatRow("Triangles rendered", stats.m_trianglesRendered, 0);
                ImGui::EndTable();
            }

            ImGui::TextUnformatted("Rendered instances per LOD");
            for (uint32_t lodIndex = 0; lodIndex < Graphics::Culling::Stats::kMaxLodCount; ++lodIndex)
            {
                if (stats.m_lodInstances[lodIndex] != 0)
                    ImGui::BulletText("LOD %u: %u", lodIndex, stats.m_lodInstances[lodIndex]);
            }
        }


        void ApplyDebugMode(const Graphics::Scene& scene) const
        {
            for (uint32_t viewIndex = 0; viewIndex < scene.GetViewCount(); ++viewIndex)
                scene.GetView(viewIndex)->SetDebugMode(m_debugMode);
        }


        void ApplyCullingSettings(const Graphics::Scene& scene) const
        {
            for (uint32_t viewIndex = 0; viewIndex < scene.GetViewCount(); ++viewIndex)
            {
                auto* culling = scene.GetView(viewIndex)->GetModules().TryFind<Graphics::Culling::ViewModule>();
                if (culling != nullptr)
                    culling->m_settings = m_cullingSettings;
            }
        }


        static void DrawFrozenFrustum(const Graphics::Scene& scene)
        {
            if (scene.GetViewCount() == 0)
                return;

            const Graphics::View& view = *scene.GetView(0);
            const auto* culling = view.GetModules().TryFind<Graphics::Culling::ViewModule>();
            if (culling == nullptr || !culling->IsFrozen())
                return;

            ImGuiViewport* mainViewport = ImGui::GetMainViewport();
            const FrustumOverlay overlay{ view.GetViewProjectionMatrix(), mainViewport->Pos, mainViewport->Size };
            overlay.Draw(*ImGui::GetBackgroundDrawList(mainViewport), culling->GetFrozenViewProjection());
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
    controls->m_cullingSettings.m_instanceOcclusionCulling = !Cli::Check("--no-occlusion-culling");
    controls->m_cullingSettings.m_meshletOcclusionCulling = !Cli::Check("--no-occlusion-culling");
    controls->m_cullingSettings.m_lodSelection = !Cli::Check("--no-lod-selection");

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
                RenderingControls::LogCullingStats(application->GetWorld());
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
