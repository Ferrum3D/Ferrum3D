#pragma once
#include <Graphics/RHI/Common/BaseTypes.h>
#include <Graphics/RHI/Vulkan/Common/Config.h>

namespace FE::Graphics::Vulkan
{
    inline VkExtent3D VKConvert(RHI::Size size)
    {
        return { static_cast<uint32_t>(size.width), static_cast<uint32_t>(size.height), static_cast<uint32_t>(size.depth) };
    }


    inline VkOffset3D VKConvert(RHI::Offset offset)
    {
        return { static_cast<int32_t>(offset.x), static_cast<int32_t>(offset.y), static_cast<int32_t>(offset.z) };
    }
} // namespace FE::Graphics::Vulkan
