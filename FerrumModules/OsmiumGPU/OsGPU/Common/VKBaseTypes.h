#pragma once
#include <OsGPU/Common/BaseTypes.h>
#include <OsGPU/Common/VKConfig.h>

namespace FE::Osmium
{
    inline VkExtent3D VKConvert(Size size)
    {
        return { static_cast<uint32_t>(size.Width), static_cast<uint32_t>(size.Height), static_cast<uint32_t>(size.Depth) };
    }

    inline VkOffset3D VKConvert(Offset offset)
    {
        return { static_cast<int32_t>(offset.X), static_cast<int32_t>(offset.Y), static_cast<int32_t>(offset.Z) };
    }
}
