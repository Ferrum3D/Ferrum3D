#pragma once
#include <FeCore/Base/Base.h>

namespace FE::Graphics::RHI
{
    enum class ShaderStage : uint32_t
    {
        kVertex,
        kPixel,
        kHull,
        kDomain,
        kGeometry,
        kCompute
    };


    enum class ShaderStageFlags : uint32_t
    {
        kNone = 0,
        kVertex = 1 << static_cast<uint32_t>(ShaderStage::kVertex),
        kPixel = 1 << static_cast<uint32_t>(ShaderStage::kPixel),
        kHull = 1 << static_cast<uint32_t>(ShaderStage::kHull),
        kDomain = 1 << static_cast<uint32_t>(ShaderStage::kDomain),
        kGeometry = 1 << static_cast<uint32_t>(ShaderStage::kGeometry),
        kCompute = 1 << static_cast<uint32_t>(ShaderStage::kCompute),
        kAll = kVertex | kHull | kDomain | kPixel | kGeometry | kCompute
    };

    FE_ENUM_OPERATORS(ShaderStageFlags);
} // namespace FE::Graphics::RHI
