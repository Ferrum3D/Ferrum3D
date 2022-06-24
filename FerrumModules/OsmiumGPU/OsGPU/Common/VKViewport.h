#pragma once
#include <OsGPU/Common/VKConfig.h>
#include <OsGPU/Common/Viewport.h>

namespace FE::Osmium
{
    inline vk::Viewport VKConvert(const Viewport& viewport)
    {
        vk::Viewport vp{};
        vp.x        = viewport.MinX;
        vp.y        = viewport.MinY;
        vp.width    = viewport.Width();
        vp.height   = viewport.Height();
        vp.minDepth = viewport.MinZ;
        vp.maxDepth = viewport.MaxZ;
        return vp;
    }

    inline vk::Rect2D VKConvert(const Scissor& scissor)
    {
        vk::Rect2D rect{};
        rect.offset = vk::Offset2D(scissor.MinX, scissor.MinY);
        rect.extent = vk::Extent2D(scissor.Width(), scissor.Height());
        return rect;
    }
} // namespace FE::Osmium
