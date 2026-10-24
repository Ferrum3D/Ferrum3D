#pragma once
#include <Framework/Entities/Entity.h>
#include <Graphics/Scene/View.h>

namespace FE::Editor
{
    bool DrawTranslationGizmo(Framework::Entity& entity, const Graphics::View& view, Vector2 origin, Vector2 size);
} // namespace FE::Editor
