#include <Framework/Entities/EntityWorld.h>
#include <GameFramework/Components/TransformComponents.h>
#include <GameFramework/Editor/Gizmos.h>
#include <imgui.h>

namespace FE::Editor
{
    namespace
    {
        bool Project(const Vector3 point, const Matrix4x4& matrix, const Vector2 origin, const Vector2 size, Vector2& screen)
        {
            const Vector4 clip = Vector4(point, 1) * matrix;
            if (clip.w <= 0.001f)
                return false;

            screen = origin + Vector2(clip.x / clip.w * 0.5f + 0.5f, 0.5f - clip.y / clip.w * 0.5f) * size;
            return true;
        }


        void SubmitTransform(Framework::Entity& entity, const Transform& transform)
        {
            Framework::EntityCommandList commands(entity.GetWorld());
            commands.ReplaceComponent(entity.GetID(), GameFramework::TransformComponent{ transform });
            entity.GetWorld().Submit(std::move(commands));
        }
    } // namespace


    bool DrawTranslationGizmo(Framework::Entity& entity, const Graphics::View& view, const Vector2 origin, const Vector2 size)
    {
        const auto* world = entity.FindComponent<const GameFramework::WorldTransformComponent>();
        const auto* local = entity.FindComponent<const GameFramework::TransformComponent>();
        if (!world || !local)
            return false;

        const Vector3 position(world->m_world.m_rows[3].m_simdVector);
        const float distance = Math::Length(position - view.GetCameraTransform().Translation());
        const float length = Math::Max(0.25f, distance * 0.12f);
        const Vector3 axes[] = { Vector3(1, 0, 0), Vector3(0, 1, 0), Vector3(0, 0, 1) };
        const ImU32 colors[] = { IM_COL32(240, 70, 70, 255), IM_COL32(80, 230, 100, 255), IM_COL32(80, 140, 255, 255) };
        Vector2 center;
        if (!Project(position, view.GetViewProjectionMatrix(), origin, size, center))
            return false;

        bool active = false;
        const float uiScale = ImGui::GetWindowDpiScale();
        const ImVec2 cursor = ImGui::GetCursorScreenPos();
        for (int axis = 0; axis < 3; ++axis)
        {
            Vector2 end;
            if (!Project(position + axes[axis] * length, view.GetViewProjectionMatrix(), origin, size, end))
                continue;

            ImGui::GetWindowDrawList()->AddLine(ImVec2(center.x, center.y), ImVec2(end.x, end.y), colors[axis], 3 * uiScale);
            ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2(end.x, end.y), 5 * uiScale, colors[axis]);
            const Vector2 handleMin = Math::Min(center, end) - Vector2(8, 8) * uiScale;
            const Vector2 handleMax = Math::Max(center, end) + Vector2(8, 8) * uiScale;
            ImGui::SetCursorScreenPos(ImVec2(handleMin.x, handleMin.y));
            ImGui::PushID(axis);
            ImGui::InvisibleButton("TranslateAxis", ImVec2(handleMax.x - handleMin.x, handleMax.y - handleMin.y));
            if (ImGui::IsItemActive())
            {
                active = true;
                const Vector2 direction = end - center;
                const float squared = Math::Dot(direction, direction);
                if (squared > 1)
                {
                    const ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left, 0);
                    const float amount = Math::Dot(Vector2(delta.x, delta.y), direction) / squared * length;
                    Vector4 motion(axes[axis] * amount, 0);
                    if (const auto* parent = entity.GetParent())
                    {
                        if (const auto* parentWorld = parent->FindComponent<const GameFramework::WorldTransformComponent>())
                            motion = motion * Math::Invert(parentWorld->m_world);
                    }

                    if (amount != 0)
                    {
                        SubmitTransform(entity,
                                        Transform::Create(local->m_local.Translation() + Vector3(motion.m_simdVector),
                                                          local->m_local.Rotation(),
                                                          local->m_local.Scale()));
                        ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
                    }
                }
            }

            ImGui::PopID();
        }

        ImGui::SetCursorScreenPos(ImVec2(cursor.x, cursor.y - ImGui::GetStyle().ItemSpacing.y));
        ImGui::Dummy(ImVec2(0, 0));
        return active;
    }
} // namespace FE::Editor
