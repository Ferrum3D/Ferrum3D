#pragma once
#include <FeCore/Base/Base.h>

namespace FE::Osmium
{
    enum class ShaderStage : uint32_t
    {
        Vertex,
        Pixel,
        Hull,
        Domain,
        Geometry,
        Compute
    };

    enum class ShaderStageFlags : uint32_t
    {
        None     = 0,
        Vertex   = 1 << static_cast<uint32_t>(ShaderStage::Vertex),
        Pixel    = 1 << static_cast<uint32_t>(ShaderStage::Pixel),
        Hull     = 1 << static_cast<uint32_t>(ShaderStage::Hull),
        Domain   = 1 << static_cast<uint32_t>(ShaderStage::Domain),
        Geometry = 1 << static_cast<uint32_t>(ShaderStage::Geometry),
        Compute  = 1 << static_cast<uint32_t>(ShaderStage::Compute),
        All      = Vertex | Hull | Domain | Pixel | Geometry | Compute
    };

    FE_ENUM_OPERATORS(ShaderStageFlags);
} // namespace FE::Osmium
