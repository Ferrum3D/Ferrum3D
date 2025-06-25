#pragma once

namespace FE::Graphics::Core
{
    enum class ShaderStage : uint32_t
    {
        kVertex = 0,
        kPixel = 1,
        kCompute = 2,
        kUndefined = kInvalidIndex,

        kGraphicsCount = kPixel + 1,
    };


    enum class ShaderStageFlags : uint32_t
    {
        kNone = 0,
        kVertex = 1 << static_cast<uint32_t>(ShaderStage::kVertex),
        kPixel = 1 << static_cast<uint32_t>(ShaderStage::kPixel),
        kCompute = 1 << static_cast<uint32_t>(ShaderStage::kCompute),
        kAllGraphics = kVertex | kPixel,
        kAll = kVertex | kPixel | kCompute,
    };

    FE_ENUM_OPERATORS(ShaderStageFlags);
} // namespace FE::Graphics::Core
