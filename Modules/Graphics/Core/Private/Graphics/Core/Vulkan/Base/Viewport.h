#pragma once
#include <FeCore/Math/Aabb.h>
#include <Graphics/Core/Vulkan/Base/Config.h>

namespace FE::Graphics::Vulkan
{
    inline VkViewport TranslateViewport(const RectF viewport)
    {
        const Vector2 size = viewport.Size();

        VkViewport vp;
        vp.x = viewport.min.x;
        vp.y = viewport.min.y + size.y;
        vp.width = size.x;
        vp.height = -size.y;
        vp.minDepth = 0.0f;
        vp.maxDepth = 1.0f;
        return vp;
    }


    inline VkRect2D TranslateScissor(const RectInt scissor)
    {
        VkRect2D rect;
        rect.offset = VkOffset2D{ scissor.min.x, scissor.min.y };
        rect.extent = VkExtent2D{ static_cast<uint32_t>(scissor.Width()), static_cast<uint32_t>(scissor.Height()) };
        return rect;
    }
} // namespace FE::Graphics::Vulkan
