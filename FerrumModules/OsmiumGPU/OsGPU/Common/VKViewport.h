#pragma once
#include <OsGPU/Common/VKConfig.h>
#include <OsGPU/Common/Viewport.h>

namespace FE::Osmium
{
    inline VkViewport VKConvert(const Viewport& viewport)
    {
        VkViewport vp{};
        vp.x        = viewport.MinX;
        vp.y        = viewport.MinY;
        vp.width    = viewport.Width();
        vp.height   = viewport.Height();
        vp.minDepth = viewport.MinZ;
        vp.maxDepth = viewport.MaxZ;
        return vp;
    }

    inline VkRect2D VKConvert(const Scissor& scissor)
    {
        VkRect2D rect{};
        rect.offset = VkOffset2D{ scissor.MinX, scissor.MinY };
        rect.extent = VkExtent2D{ static_cast<uint32_t>(scissor.Width()), static_cast<uint32_t>(scissor.Height()) };
        return rect;
    }
} // namespace FE::Osmium
