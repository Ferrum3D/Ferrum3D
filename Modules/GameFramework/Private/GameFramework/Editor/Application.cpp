#include <Core/IO/Artifact.h>
#include <Core/Utils/Icons.h>
#include <Framework/Application/Core/PlatformMonitor.h>
#include <Framework/Entities/WorldTimeService.h>
#include <Framework/Input/WorldInputService.h>
#include <GameFramework/Components/CameraComponent.h>
#include <GameFramework/Components/MeshComponent.h>
#include <GameFramework/Components/TransformComponents.h>
#include <GameFramework/Editor/Application.h>
#include <GameFramework/Editor/CameraMovementSystem.h>
#include <GameFramework/Editor/Controls.h>
#include <GameFramework/Editor/Gizmos.h>
#include <GameFramework/Editor/InputAssets.h>
#include <GameFramework/Editor/Theme.h>
#include <GameFramework/Services/WorldGraphicsSceneService.h>
#include <GameFramework/Systems/CameraSystem.h>
#include <GameFramework/Systems/MeshSystem.h>
#include <GameFramework/Systems/TransformationSystem.h>
#include <Graphics/Renderer.h>
#include <imgui.h>
#include <imgui_internal.h>

namespace FE::Editor
{
    namespace
    {
        //! Edits are submitted before the next world update and commit at its start; one extra frame is a safety margin.
        constexpr int32_t kComponentEditSettleFrames = 2;


        template<class T>
        void RequireSystem(Framework::EntityWorldAsset& definition)
        {
            const auto id = Rtti::GetTypeID<T>();
            if (festd::find(definition.m_systems, id) == definition.m_systems.end())
                definition.m_systems.push_back(id);
        }


        template<class T>
        void RequireService(Framework::EntityWorldAsset& definition)
        {
            for (const auto& service : definition.m_services)
            {
                if (service.m_type == Rtti::GetTypeID<T>())
                    return;
            }

            definition.m_services.push_back(Rtti::GetTypeID<T>());
        }
    } // namespace


    bool Application::Initialize(const GameFramework::ApplicationSettings& settings)
    {
        m_assetDirectory = IO::GetAbsolutePath(settings.m_assetDirectory);
        auto editorSettings = settings;
        editorSettings.m_startPaused = true;
        if (!GameFramework::Application::Initialize(editorSettings))
            return false;

        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        ImGui::GetIO().IniFilename = "editor.ini";
        const auto monitors = m_platformApplication->GetMonitors();
        const uint32_t monitor = m_mainWindow->GetMonitorIndex();
        m_uiScale = monitor < monitors.size() ? monitors[monitor].m_dpiScale : 1.0f;
        ApplyTheme(m_uiScale);
        m_assets.Refresh(m_assetDirectory);
        return true;
    }


    void Application::PrepareWorldDefinition(Framework::EntityWorldAsset& definition)
    {
        RequireService<Framework::WorldTimeService>(definition);
        RequireService<Input::WorldInputService>(definition);
        RequireService<GameFramework::WorldGraphicsSceneService>(definition);
        RequireSystem<GameFramework::TransformationSystem>(definition);
        RequireSystem<GameFramework::CameraSystem>(definition);
        RequireSystem<GameFramework::MeshSystem>(definition);

        m_initialCamera = Transform::Translation(Vector3(0, 2, -6));
        for (const auto& registry : definition.m_registries)
        {
            for (const auto& entity : registry.m_entities.m_entities)
            {
                bool camera = false;
                const GameFramework::TransformComponent* transform = nullptr;
                for (const auto& component : entity.m_components)
                {
                    camera |= component.Is<GameFramework::CameraComponent>();
                    if (const auto* candidate = component.TryGet<GameFramework::TransformComponent>())
                        transform = candidate;
                }

                if (camera && transform)
                {
                    m_initialCamera = transform->m_local;
                    return;
                }
            }
        }
    }


    void Application::OnWorldLoaded()
    {
        auto& world = GetWorld();
        world.AddSystem(m_cameraMovement);
        world.AddSystem(m_selection);
        m_editorSystemsRegistered = true;
        m_inputContext =
            world.FindService<Input::WorldInputService>()->Activate(IO::Link<Input::MappingTable>(InputAssets::kMappings),
                                                                    100,
                                                                    true);
        auto& registry = world.CreateRegistry();
        registry.SetTransient(true);

        m_cameraUuid = Uuid::Random();

        Framework::EntityCommandList commands(world);
        const auto camera = commands.CreateEntity(registry, Env::Name("Editor Camera"), m_cameraUuid);
        commands.AddComponent(camera, GameFramework::TransformComponent{ m_initialCamera });
        commands.AddComponent<GameFramework::CameraComponent>(camera);
        CameraMovementComponent movement;
        movement.m_movement.m_moveAction = IO::Link<Input::Action>(InputAssets::kMove);
        movement.m_movement.m_elevateAction = IO::Link<Input::Action>(InputAssets::kElevate);
        movement.m_movement.m_lookAction = IO::Link<Input::Action>(InputAssets::kLook);
        movement.m_speedUpAction = IO::Link<Input::Action>(InputAssets::kSpeedUp);
        commands.AddComponent(camera, movement);
        commands.SetActive(camera, true);
        world.Submit(std::move(commands));

        // Startup may still be waiting for world assets; normal paused updates continue its lifecycle.
        world.CommitBootstrap();
    }


    void Application::OnWorldUnloading()
    {
        m_serviceSettingsError.clear();

        // Entity IDs of the next world may reuse slots, so pending edits of this one are dropped.
        m_selected = {};
        m_componentEdit = {};
        m_gizmo = {};
        m_cameraUuid = Uuid::kNull;
        if (m_editorSystemsRegistered)
        {
            auto& world = GetWorld();
            world.FindService<Input::WorldInputService>()->Deactivate(m_inputContext);
            world.RemoveSystem(m_selection);
            world.RemoveSystem(m_cameraMovement);
            m_editorSystemsRegistered = false;
        }

        m_selection.Reset();
        m_viewportTexture.Reset();
    }


    Graphics::View* Application::GetEditorView()
    {
        const auto* camera = GetWorld().Find(m_cameraUuid);
        if (!camera)
            return nullptr;

        const auto* component = camera->FindComponent<const GameFramework::CameraComponent>();
        return component ? component->m_view.Get() : nullptr;
    }


    bool Application::Pick(const Vector2 position)
    {
        auto* view = GetEditorView();
        if (!view || position.x < 0 || position.y < 0 || position.x >= 1 || position.y >= 1)
            return false;

        auto* selection = view->GetModules().TryFind<SelectionViewModule>();
        if (!selection || selection->IsPickPending())
            return false;

        selection->RequestPick(Vector2UInt(static_cast<uint32_t>(position.x * m_viewportSize.x),
                                           static_cast<uint32_t>(position.y * m_viewportSize.y)));
        return true;
    }


    void Application::BeforeWorldUpdate()
    {
        SubmitPendingEdits();

        // The frame loop already joined scene reads; view modules finished pass setup before the scene was released.
        auto* view = GetEditorView();
        if (!view)
            return;

        if (!view->GetModules().Contains<SelectionViewModule>())
        {
            view->GetModules().Add<SelectionViewModule>();
            view->GetModules().Find<SelectionViewModule>().Initialize(GetPipelineFactory());
        }

        uint32_t picked = 0;
        if (view->GetModules().Find<SelectionViewModule>().ConsumePick(picked))
        {
            m_selected = m_selection.FindEntity(picked);
            if (!GetWorld().Find(m_selected))
                m_selected = {};
        }

        m_cameraMovement.m_inputEnabled = m_viewportHovered && m_viewportFocused;
        m_cameraMovement.m_viewportSize = m_viewportSize;
        m_selection.m_selected = m_selected;
        m_selection.m_view = view;
    }


    void Application::ScheduleWorldUpdate(Framework::EntityWorld& world)
    {
        GameFramework::Application::ScheduleWorldUpdate(world);
        world.SchedulePhase(Phases::Selection);
    }


    void Application::DrawEntity(Framework::Entity& entity, const uint32_t depth)
    {
        if (entity.GetRegistry().IsTransient() || depth > 128)
            return;

        ImGui::PushID(Fmt::FixedFormat("{}", entity.GetUuid()).c_str());
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (!entity.GetFirstChild())
            flags |= ImGuiTreeNodeFlags_Leaf;

        if (entity.GetID() == m_selected)
            flags |= ImGuiTreeNodeFlags_Selected;

        const bool expanded = ImGui::TreeNodeEx("##Entity",
                                                flags,
                                                FE_ICON_ENTITY " %s",
                                                entity.GetName().IsValid() ? entity.GetName().c_str() : "Entity");
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
            m_selected = entity.GetID();

        if (expanded)
        {
            for (auto* child = entity.GetFirstChild(); child; child = child->GetNextSibling())
                DrawEntity(*child, depth + 1);

            ImGui::TreePop();
        }

        ImGui::PopID();
    }


    void Application::DrawWorldWindow()
    {
        if (ImGui::Begin("World"))
        {
            GetWorld().TraverseRoots([this](Framework::Entity& entity) {
                DrawEntity(entity, 0);
                return true;
            });
        }

        ImGui::End();
    }


    void Application::DrawEntityWindow()
    {
        if (ImGui::Begin("Entity"))
        {
            if (auto* entity = GetWorld().Find(m_selected))
            {
                for (const auto* component : GetWorld().GetComponents(*entity))
                    DrawComponent(*entity, *component);
            }
        }

        ImGui::End();

        // Live values show the last edit once its replacement committed and no control holds the session.
        const bool isSettled = ImGui::GetFrameCount() - m_componentEdit.m_lastEditFrame > kComponentEditSettleFrames;
        const bool canEndEdit = isSettled && !m_componentEdit.m_isPending && !ImGui::IsAnyItemActive();
        if (m_componentEdit.m_value.HasValue() && canEndEdit)
            m_componentEdit = {};
    }


    void Application::DrawComponent(Framework::Entity& entity, const Framework::EntityComponentInfo& component)
    {
        const Rtti::Type& type = *component.m_type;
        const auto label = Fmt::FixedFormat("{} {}", GetIcon(&type), GetDisplayName(type));
        ImGui::PushID(Fmt::FixedFormat("{}", type.m_id).c_str());
        const auto popID = festd::defer([] {
            ImGui::PopID();
        });

        if (!ImGui::CollapsingHeader(label.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
            return;

        // Controls draw from the pending edit while one exists, because submitted replacements commit later.
        const void* live = entity.FindComponent(type.m_id);
        const bool isEditing = m_componentEdit.m_entity == entity.GetID() && m_componentEdit.m_type == &type;
        const void* value = isEditing ? m_componentEdit.m_value.GetValue() : live;
        const bool readOnly = component.m_policy.m_transient || !type.m_copyConstructor || !type.m_destructor;

        InspectorEdit edit;
        const InspectorContext context{ m_assets.GetAssets() };
        if (!DrawReflectedValue(type, value, edit, context, readOnly, live))
            return;

        if (!isEditing)
        {
            // Another component's edit is submitted before the next world update; replacing it now would drop it.
            if (m_componentEdit.m_isPending)
                return;

            Rtti::Any started;
            if (!started.Copy(type, live))
                return;

            m_componentEdit.m_entity = entity.GetID();
            m_componentEdit.m_type = &type;
            m_componentEdit.m_value = std::move(started);
        }

        void* candidate = m_componentEdit.m_value.GetValue();
        if (!edit.Apply(type, candidate))
            return;

        m_componentEdit.m_lastEditFrame = ImGui::GetFrameCount();
        m_componentEdit.m_isPending = !type.m_validate || type.m_validate(candidate);
    }


    void Application::SubmitPendingEdits()
    {
        auto& world = GetWorld();
        Framework::EntityCommandList commands(world);
        bool hasCommands = m_gizmo.RecordPendingEdit(world, commands);

        ComponentEdit& edit = m_componentEdit;
        if (edit.m_isPending && world.Find(edit.m_entity))
            hasCommands |= commands.ReplaceComponent(edit.m_entity, *edit.m_type, edit.m_value.GetValue());

        edit.m_isPending = false;

        // One list per world update: independent lists replacing the same component conflict at commit.
        if (hasCommands)
            world.Submit(std::move(commands));
    }


    void Application::DrawServicesWindow()
    {
        if (ImGui::Begin("Services"))
        {
            for (Framework::WorldService* service : GetWorld().GetServices())
            {
                const Rtti::Type& type = service->RTTI_GetDynamicType();
                ImGui::PushID(service);
                const auto label = Fmt::FixedFormat("{}", GetDisplayName(type));
                if (ImGui::CollapsingHeader(label.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
                {
                    void* object = service->RTTI_GetDynamicObject();
                    Rtti::Any settings;
                    if (type.m_captureConfiguration)
                        settings = type.m_captureConfiguration(object);

                    if (!settings.HasValue())
                    {
                        ImGui::TextDisabled("No settings");
                    }
                    else
                    {
                        const bool readOnly = type.m_applyConfiguration == nullptr;
                        if (readOnly)
                            ImGui::TextDisabled("These settings apply when the world starts.");

                        const Rtti::Type& configurationType = *settings.GetType();
                        InspectorEdit edit;
                        const InspectorContext context{ m_assets.GetAssets() };
                        if (DrawReflectedValue(configurationType, settings.GetValue(), edit, context, readOnly) && !readOnly)
                        {
                            const bool valid = edit.Apply(configurationType, settings.GetValue())
                                && (!configurationType.m_validate || configurationType.m_validate(settings.GetValue()));
                            if (valid && type.m_applyConfiguration(object, settings))
                                m_serviceSettingsError.clear();
                            else
                                m_serviceSettingsError = "Settings were rejected. Check the parameter ranges.";
                        }
                    }
                }

                ImGui::PopID();
            }

            if (!m_serviceSettingsError.empty())
                ImGui::TextColored(ImVec4(1, 0.4f, 0.3f, 1), "%s", m_serviceSettingsError.c_str());
        }

        ImGui::End();
    }


    void Application::DrawViewport()
    {
        m_viewportHovered = false;
        m_viewportFocused = false;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        if (ImGui::Begin("Main Viewport") && GetEditorView())
        {
            const ImVec2 available = ImGui::GetContentRegionAvail();
            if (available.x >= 1 && available.y >= 1)
            {
                m_viewportSize = Vector2UInt(static_cast<uint32_t>(available.x), static_cast<uint32_t>(available.y));
                if (!m_viewportTexture || m_viewportTexture->GetDesc().GetSize2D() != m_viewportSize)
                {
                    Graphics::Renderer::Get().WaitForRender();
                    m_viewportTexture = Graphics::Core::Texture::Create(GetDevice(),
                                                                        "EditorViewport",
                                                                        Graphics::Core::Format::kR8G8B8A8_UNORM,
                                                                        m_viewportSize);
                }

                const ImVec2 origin = ImGui::GetCursorScreenPos();
                ImGui::Image(static_cast<ImTextureID>(reinterpret_cast<uintptr_t>(m_viewportTexture.Get())), available);
                m_viewportHovered = ImGui::IsItemHovered();
                m_viewportFocused = ImGui::IsWindowFocused();
                if (m_viewportHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                {
                    ImGui::SetWindowFocus();
                    m_viewportFocused = true;
                }

                if (auto* view = GetEditorView())
                {
                    bool gizmo = false;
                    if (auto* selected = GetWorld().Find(m_selected))
                        gizmo = m_gizmo.Draw(*selected, *view, Vector2(origin.x, origin.y), Vector2(available.x, available.y));

                    if (m_viewportHovered && !gizmo && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                    {
                        const ImVec2 mouse = ImGui::GetMousePos();
                        if (auto* selection = view->GetModules().TryFind<SelectionViewModule>())
                        {
                            selection->RequestPick(Vector2UInt(static_cast<uint32_t>(mouse.x - origin.x),
                                                               static_cast<uint32_t>(mouse.y - origin.y)));
                        }
                    }
                }

                ImGui::GetWindowDrawList()->AddText(
                    ImVec2(origin.x + 12 * ImGui::GetWindowDpiScale(), origin.y + 12 * ImGui::GetWindowDpiScale()),
                    IM_COL32_WHITE,
                    "RMB + WASD: fly | Q/E: down/up | Shift: fast\nClick: select | Drag axes: move");
            }
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }


    void Application::DrawOpenWorld()
    {
        if (m_openWorldDialog)
        {
            ImGui::OpenPopup("Open World");
            m_openWorldDialog = false;
        }

        ImGui::SetNextWindowSize(ImVec2(640 * m_uiScale, 440 * m_uiScale), ImGuiCond_Appearing);
        if (!ImGui::BeginPopupModal("Open World", nullptr))
            return;

        ImGui::InputTextWithHint("##WorldPath", "World .asset path", m_worldPath, sizeof(m_worldPath));
        ImGui::SameLine();
        if (ImGui::Button("Open"))
        {
            IO::Path path(m_worldPath);
            if (IO::PathView(path).is_relative())
                path = m_assetDirectory / path;

            const IO::AssetID world = AssetBrowser::ReadAssetFile(path, Rtti::GetTypeID<Framework::EntityWorldAsset>());
            m_openFailed = !world.IsValid() || !OpenWorld(world);
            if (!m_openFailed)
                ImGui::CloseCurrentPopup();
        }

        if (m_openFailed)
            ImGui::TextColored(ImVec4(1, 0.4f, 0.3f, 1), "Choose a world .asset file with cooked artifacts in this project.");

        ImGui::Separator();
        if (const auto world = m_assets.Draw(Rtti::GetTypeID<Framework::EntityWorldAsset>(), true); world.IsValid())
        {
            OpenWorld(world);
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::Button("Cancel"))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }


    void Application::DrawImGuiWindows()
    {
        const float dpiScale = ImGui::GetMainViewport()->DpiScale;
        if (m_uiScale != dpiScale)
        {
            m_uiScale = dpiScale;
            ApplyTheme(m_uiScale);
        }

        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem(FE_ICON_OPEN " Open...", "Ctrl+O"))
                    m_openWorldDialog = true;

                if (ImGui::MenuItem(FE_ICON_REFRESH " Refresh Assets"))
                    m_assets.Refresh(m_assetDirectory);

                ImGui::EndMenu();
            }

            DrawGameEditorMenu();
            ImGui::EndMainMenuBar();
        }

        // Routed shortcuts are not delivered while a text field owns the keyboard.
        if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_O, ImGuiInputFlags_RouteGlobal))
            m_openWorldDialog = true;

        const ImGuiID dock = ImGui::GetID("FerrumEditorDock2");
        const bool needsLayout = ImGui::DockBuilderGetNode(dock) == nullptr;
        ImGui::DockSpaceOverViewport(dock);
        if (needsLayout)
        {
            ImGui::DockBuilderSetNodeSize(dock, ImGui::GetMainViewport()->WorkSize);
            ImGuiID center = dock;
            ImGuiID left = ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, 0.26f, nullptr, &center);
            const ImGuiID bottom = ImGui::DockBuilderSplitNode(center, ImGuiDir_Down, 0.3f, nullptr, &center);
            const ImGuiID inspector = ImGui::DockBuilderSplitNode(left, ImGuiDir_Down, 0.6f, nullptr, &left);
            ImGui::DockBuilderDockWindow("World", left);
            ImGui::DockBuilderDockWindow("Entity", inspector);
            ImGui::DockBuilderDockWindow("Services", inspector);
            ImGui::DockBuilderDockWindow("Asset Browser", bottom);
            ImGui::DockBuilderDockWindow("Main Viewport", center);
            ImGui::DockBuilderFinish(dock);
        }

        DrawWorldWindow();
        DrawEntityWindow();
        DrawServicesWindow();
        if (ImGui::Begin("Asset Browser"))
        {
            if (const auto world = m_assets.Draw(Rtti::GetTypeID<Framework::EntityWorldAsset>()); world.IsValid())
                OpenWorld(world);
        }

        ImGui::End();
        DrawViewport();
        DrawOpenWorld();
        DrawGameEditorWindows();

        const bool popup = ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId);
        if (popup)
        {
            m_viewportHovered = false;
            m_viewportFocused = false;
        }

        m_imgui->SetInputPassthrough(m_viewportHovered, m_viewportHovered && m_viewportFocused);
    }


    void Application::BuildRenderRequest(Graphics::RenderRequest& request)
    {
        request.AddViewport(GetMainViewport());
        if (auto* view = GetEditorView(); view && m_viewportTexture)
            request.AddView(view, m_viewportTexture.Get());

        AddImGuiViews(request);
    }
} // namespace FE::Editor
