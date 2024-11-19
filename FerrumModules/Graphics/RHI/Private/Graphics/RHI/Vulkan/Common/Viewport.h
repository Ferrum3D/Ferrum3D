#pragma once
#include <Graphics/RHI/Common/Viewport.h>
#include <Graphics/RHI/Vulkan/Common/Config.h>

namespace FE::Graphics::Vulkan
{
    inline VkViewport VKConvert(const RHI::Viewport& viewport)
    {
        VkViewport vp{};
        vp.x = viewport.minX;
        vp.y = viewport.minY;
        vp.width = viewport.Width();
        vp.height = viewport.Height();
        vp.minDepth = viewport.minZ;
        vp.maxDepth = viewport.maxZ;
        return vp;
    }

    inline VkRect2D VKConvert(const RHI::Scissor& scissor)
    {
        VkRect2D rect{};
        rect.offset = VkOffset2D{ scissor.minX, scissor.minY };
        rect.extent = VkExtent2D{ static_cast<uint32_t>(scissor.Width()), static_cast<uint32_t>(scissor.Height()) };
        return rect;
    }
} // namespace FE::Graphics::Vulkan
