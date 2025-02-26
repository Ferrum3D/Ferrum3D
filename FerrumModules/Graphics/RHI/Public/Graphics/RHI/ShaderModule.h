#pragma once
#include <Graphics/RHI/DeviceObject.h>
#include <Graphics/RHI/ShaderStage.h>

namespace FE::Graphics::RHI
{
    struct ShaderReflection;

    struct ShaderModuleDesc final
    {
        Env::Name m_entryPoint;
        ShaderStage m_stage = ShaderStage::kVertex;
    };


    struct ShaderModule : public DeviceObject
    {
        FE_RTTI_Class(ShaderModule, "0040A2EF-9D25-42AC-9A95-B3F8D4288E49");

        ~ShaderModule() override = default;

        [[nodiscard]] virtual const ShaderModuleDesc& GetDesc() const = 0;
        [[nodiscard]] virtual ShaderReflection* GetReflection() = 0;
    };
} // namespace FE::Graphics::RHI
