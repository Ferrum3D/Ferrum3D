#pragma once
#include <festd/string.h>

namespace FE::Graphics::Core
{
    enum class ShaderStage : uint32_t
    {
        kVertex = 0,
        kAmplification = 1,
        kMesh = 2,
        kPixel = 3,
        kCompute = 4,
        kUndefined = kInvalidIndex,

        kGraphicsCount = kPixel + 1,
    };


    enum class ShaderStageFlags : uint32_t
    {
        kNone = 0,
        kVertex = 1 << static_cast<uint32_t>(ShaderStage::kVertex),
        kAmplification = 1 << static_cast<uint32_t>(ShaderStage::kAmplification),
        kMesh = 1 << static_cast<uint32_t>(ShaderStage::kMesh),
        kPixel = 1 << static_cast<uint32_t>(ShaderStage::kPixel),
        kCompute = 1 << static_cast<uint32_t>(ShaderStage::kCompute),
        kAllGraphics = kVertex | kPixel,
        kAll = kVertex | kPixel | kCompute,
    };

    FE_ENUM_OPERATORS(ShaderStageFlags);


    inline ShaderStage GetShaderStageFromName(const festd::string_view name)
    {
        if (name.ends_with(".vs"))
            return ShaderStage::kVertex;
        if (name.ends_with(".as"))
            return ShaderStage::kAmplification;
        if (name.ends_with(".ms"))
            return ShaderStage::kMesh;
        if (name.ends_with(".ps"))
            return ShaderStage::kPixel;
        if (name.ends_with(".cs"))
            return ShaderStage::kCompute;

        return ShaderStage::kUndefined;
    }
} // namespace FE::Graphics::Core
