#pragma once
#include <FeCore/Base/Base.h>

namespace FE
{
    enum class ShaderStage : UInt32
    {
        Vertex,
        Tessellation,
        Pixel,
        Geometry,
        Compute
    };

    enum class ShaderStageFlags : UInt32
    {
        None = 0,
        Vertex = 1 << static_cast<UInt32>(ShaderStage::Vertex),
        Tessellation = 1 << static_cast<UInt32>(ShaderStage::Tessellation),
        Pixel = 1 << static_cast<UInt32>(ShaderStage::Pixel),
        Geometry = 1 << static_cast<UInt32>(ShaderStage::Geometry),
        Compute = 1 << static_cast<UInt32>(ShaderStage::Compute),
        All = Vertex | Tessellation | Pixel | Compute
    };

    FE_ENUM_OPERATORS(ShaderStageFlags);
}
