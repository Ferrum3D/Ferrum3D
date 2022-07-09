#pragma once
#include <OsGPU/Common/BaseTypes.h>
#include <OsGPU/Common/VKConfig.h>

namespace FE::Osmium
{
    inline VkExtent3D VKConvert(Size size)
    {
        return { static_cast<UInt32>(size.Width), static_cast<UInt32>(size.Height), static_cast<UInt32>(size.Depth) };
    }

    inline VkOffset3D VKConvert(Offset offset)
    {
        return { static_cast<Int32>(offset.X), static_cast<Int32>(offset.Y), static_cast<Int32>(offset.Z) };
    }
}
