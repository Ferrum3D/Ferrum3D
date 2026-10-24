#pragma once
#include <Framework/Entities/Entity.h>
#include <Framework/Entities/EntityCommandList.h>
#include <Graphics/Scene/View.h>

namespace FE::Editor
{
    //! Axis translation handles for one entity.
    //!
    //! Transform replacements commit at the next world update, so a drag applies its total offset to the
    //! translation captured when it started instead of accumulating per-frame deltas onto a stale value.
    struct TranslationGizmo final
    {
        //! @brief Draw the handles into the current window; returns true while a handle is dragged.
        bool Draw(Framework::Entity& entity, const Graphics::View& view, Vector2 origin, Vector2 size);

        //! @brief Record the latest drag result once per world update; independent replacements of one target conflict.
        bool RecordPendingEdit(Framework::EntityWorld& world, Framework::EntityCommandList& commands);

    private:
        void BeginDrag(const Framework::Entity& entity, const Transform& local, int32_t axis, Vector3 worldMotion,
                       Vector2 screenDirection);
        void UpdateDrag(const Transform& local);

        Framework::EntityID m_entity;
        Transform m_pendingTransform;
        bool m_hasPendingEdit = false;
        Vector3 m_startTranslation;
        //! Parent-space translation for a drag of one screen direction length.
        Vector3 m_parentMotion;
        Vector2 m_screenDirection;
        float m_recordedAmount = 0;
        int32_t m_axis = -1;
    };
} // namespace FE::Editor
