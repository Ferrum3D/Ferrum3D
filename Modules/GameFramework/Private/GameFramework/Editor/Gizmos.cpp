#include <Framework/Entities/EntityWorld.h>
#include <GameFramework/Components/TransformComponents.h>
#include <GameFramework/Editor/Gizmos.h>
#include <imgui.h>

namespace FE::Editor
{
    namespace
    {
        constexpr int32_t kNoAxis = -1;

        //! Screen-space handles shorter than one pixel cannot map mouse motion to an axis.
        constexpr float kMinScreenDirectionLengthSquared = 1.0f;


        bool Project(const Vector3 point, const Matrix4x4& matrix, const Vector2 origin, const Vector2 size, Vector2& screen)
        {
            const Vector4 clip = Vector4(point, 1) * matrix;
            if (clip.w <= 0.001f)
                return false;

            screen = origin + Vector2(clip.x / clip.w * 0.5f + 0.5f, 0.5f - clip.y / clip.w * 0.5f) * size;
            return true;
        }
    } // namespace


    bool TranslationGizmo::Draw(Framework::Entity& entity, const Graphics::View& view, const Vector2 origin, const Vector2 size)
    {
        if (m_entity != entity.GetID())
        {
            m_entity = entity.GetID();
            m_axis = kNoAxis;
        }

        const auto* world = entity.FindComponent<const GameFramework::WorldTransformComponent>();
        const auto* local = entity.FindComponent<const GameFramework::TransformComponent>();
        if (!world || !local)
        {
            m_axis = kNoAxis;
            return false;
        }

        const Vector3 position(world->m_world.m_rows[3].m_simdVector);
        const float distance = Math::Length(position - view.GetCameraTransform().Translation());
        const float length = Math::Max(0.25f, distance * 0.12f);
        const Vector3 axes[] = { Vector3(1, 0, 0), Vector3(0, 1, 0), Vector3(0, 0, 1) };
        const ImU32 colors[] = { IM_COL32(240, 70, 70, 255), IM_COL32(80, 230, 100, 255), IM_COL32(80, 140, 255, 255) };

        Vector2 center;
        if (!Project(position, view.GetViewProjectionMatrix(), origin, size, center))
        {
            m_axis = kNoAxis;
            return false;
        }

        bool active = false;
        const float uiScale = ImGui::GetWindowDpiScale();
        const ImVec2 cursor = ImGui::GetCursorScreenPos();
        for (int32_t axis = 0; axis < 3; ++axis)
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
            if (ImGui::IsItemActivated())
                BeginDrag(entity, local->m_local, axis, axes[axis] * length, end - center);

            if (ImGui::IsItemActive() && m_axis == axis)
            {
                active = true;
                UpdateDrag(local->m_local);
            }

            ImGui::PopID();
        }

        if (!active)
            m_axis = kNoAxis;

        ImGui::SetCursorScreenPos(ImVec2(cursor.x, cursor.y - ImGui::GetStyle().ItemSpacing.y));
        ImGui::Dummy(ImVec2(0, 0));
        return active;
    }


    void TranslationGizmo::BeginDrag(const Framework::Entity& entity, const Transform& local, const int32_t axis,
                                     const Vector3 worldMotion, const Vector2 screenDirection)
    {
        Vector4 motion(worldMotion, 0);
        if (const auto* parent = entity.GetParent())
        {
            if (const auto* parentWorld = parent->FindComponent<const GameFramework::WorldTransformComponent>())
                motion = motion * Math::Invert(parentWorld->m_world);
        }

        m_axis = axis;
        m_startTranslation = local.Translation();
        m_parentMotion = Vector3(motion.m_simdVector);
        m_screenDirection = screenDirection;
        m_recordedAmount = 0;
    }


    bool TranslationGizmo::RecordPendingEdit(Framework::EntityWorld& world, Framework::EntityCommandList& commands)
    {
        if (!m_hasPendingEdit)
            return false;

        m_hasPendingEdit = false;
        if (!world.Find(m_entity))
            return false;

        return commands.ReplaceComponent(m_entity, GameFramework::TransformComponent{ m_pendingTransform });
    }


    void TranslationGizmo::UpdateDrag(const Transform& local)
    {
        const float lengthSquared = Math::Dot(m_screenDirection, m_screenDirection);
        if (lengthSquared <= kMinScreenDirectionLengthSquared)
            return;

        const ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left, 0);
        const float amount = Math::Dot(Vector2(delta.x, delta.y), m_screenDirection) / lengthSquared;
        if (amount == m_recordedAmount)
            return;

        m_recordedAmount = amount;
        m_pendingTransform = Transform::Create(m_startTranslation + m_parentMotion * amount, local.Rotation(), local.Scale());
        m_hasPendingEdit = true;
    }
} // namespace FE::Editor
