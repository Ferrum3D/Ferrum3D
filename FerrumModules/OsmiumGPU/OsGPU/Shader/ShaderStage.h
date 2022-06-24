#pragma once
#include <FeCore/Base/Base.h>

namespace FE::Osmium
{
    enum class ShaderStage : UInt32
    {
        Vertex,
        Pixel,
        Hull,
        Domain,
        Geometry,
        Compute
    };

    enum class ShaderStageFlags : UInt32
    {
        None     = 0,
        Vertex   = 1 << static_cast<UInt32>(ShaderStage::Vertex),
        Pixel    = 1 << static_cast<UInt32>(ShaderStage::Pixel),
        Hull     = 1 << static_cast<UInt32>(ShaderStage::Hull),
        Domain   = 1 << static_cast<UInt32>(ShaderStage::Domain),
        Geometry = 1 << static_cast<UInt32>(ShaderStage::Geometry),
        Compute  = 1 << static_cast<UInt32>(ShaderStage::Compute),
        All      = Vertex | Hull | Domain | Pixel | Geometry | Compute
    };

    FE_ENUM_OPERATORS(ShaderStageFlags);
} // namespace FE::Osmium
