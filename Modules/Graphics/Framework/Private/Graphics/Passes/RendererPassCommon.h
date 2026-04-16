#pragma once
#include <Graphics/Core/Texture.h>
#include <Graphics/Core/Viewport.h>
#include <Graphics/Database/Database.h>
#include <Graphics/Scene/Scene.h>
#include <Graphics/Scene/View.h>

namespace FE::Graphics::Internal
{
    struct RendererViewData final
    {
        Scene* m_scene = nullptr;
        View* m_view = nullptr;
        Core::Viewport* m_viewport = nullptr;
        Core::Texture* m_mainColorTarget = nullptr;
        Core::Texture* m_mainDepthTarget = nullptr;
        RectF m_viewportRect{ kForceInit };
        DB::Database* m_database = nullptr;
    };
} // namespace FE::Graphics::Internal
