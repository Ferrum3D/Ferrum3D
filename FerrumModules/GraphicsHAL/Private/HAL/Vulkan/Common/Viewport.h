#pragma once
#include <HAL/Vulkan/Common/Config.h>
#include <HAL/Common/Viewport.h>

namespace FE::Graphics::Vulkan
{
    inline VkViewport VKConvert(const HAL::Viewport& viewport)
    {
        VkViewport vp{};
        vp.x = viewport.MinX;
        vp.y = viewport.MinY;
        vp.width = viewport.Width();
        vp.height = viewport.Height();
        vp.minDepth = viewport.MinZ;
        vp.maxDepth = viewport.MaxZ;
        return vp;
    }

    inline VkRect2D VKConvert(const HAL::Scissor& scissor)
    {
        VkRect2D rect{};
        rect.offset = VkOffset2D{ scissor.MinX, scissor.MinY };
        rect.extent = VkExtent2D{ static_cast<uint32_t>(scissor.Width()), static_cast<uint32_t>(scissor.Height()) };
        return rect;
    }
} // namespace FE::Graphics::Vulkan
