#pragma once

namespace FE::Graphics::Core
{
    enum class ShaderStage : uint32_t
    {
        kVertex = 0,
        kPixel = 1,
        kHull = 2,
        kDomain = 3,
        kGeometry = 4,
        kCompute = 5,
        kUndefined = Constants::kMaxU32,

        kGraphicsCount = kGeometry + 1,
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


    inline const char* GetShaderEntryPointName(const ShaderStage shaderStage)
    {
        switch (shaderStage)
        {
        case ShaderStage::kVertex:
            return "main_vs";
        case ShaderStage::kPixel:
            return "main_ps";
        case ShaderStage::kHull:
            return "main_hs";
        case ShaderStage::kDomain:
            return "main_ds";
        case ShaderStage::kGeometry:
            return "main_gs";
        case ShaderStage::kCompute:
            return "main_cs";

        case ShaderStage::kUndefined:
            FE_Assert(false, "Only shader header files can have an undefined stage");
            return nullptr;

        default:
            FE_Assert(false, "Invalid shader stage");
            return nullptr;
        }
    }
} // namespace FE::Graphics::Core
