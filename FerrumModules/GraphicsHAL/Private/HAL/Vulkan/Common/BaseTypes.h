#pragma once
#include <HAL/Common/BaseTypes.h>
#include <HAL/Vulkan/Common/Config.h>

namespace FE::Graphics::Vulkan
{
    inline VkExtent3D VKConvert(HAL::Size size)
    {
        return { static_cast<uint32_t>(size.Width), static_cast<uint32_t>(size.Height), static_cast<uint32_t>(size.Depth) };
    }

    inline VkOffset3D VKConvert(HAL::Offset offset)
    {
        return { static_cast<int32_t>(offset.X), static_cast<int32_t>(offset.Y), static_cast<int32_t>(offset.Z) };
    }
} // namespace FE::Graphics::Vulkan
