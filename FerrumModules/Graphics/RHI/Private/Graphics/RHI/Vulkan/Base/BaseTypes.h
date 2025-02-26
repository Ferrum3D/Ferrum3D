#pragma once
#include <FeCore/Math/Vector3Int.h>
#include <Graphics/RHI/Base/BaseTypes.h>
#include <Graphics/RHI/Vulkan/Base/Config.h>

namespace FE::Graphics::Vulkan
{
    inline VkExtent3D VKConvertExtent(const PackedVector3UInt size)
    {
        return VkExtent3D{ size.x, size.y, size.z };
    }


    inline VkOffset3D VKConvertOffset(const PackedVector3Int offset)
    {
        return VkOffset3D{ offset.x, offset.y, offset.z };
    }
} // namespace FE::Graphics::Vulkan
