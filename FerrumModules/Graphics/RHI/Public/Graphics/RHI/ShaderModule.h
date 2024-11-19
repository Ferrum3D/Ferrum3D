#pragma once
#include <FeCore/Containers/ByteBuffer.h>
#include <FeCore/Logging/Trace.h>
#include <FeCore/Strings/FixedString.h>
#include <Graphics/RHI/DeviceObject.h>
#include <Graphics/RHI/ShaderStage.h>

namespace FE::Graphics::RHI
{
    struct ShaderReflection;

    struct ShaderModuleDesc final
    {
        festd::span<const uint8_t> m_byteCode;
        Env::Name m_entryPoint;
        ShaderStage m_stage = ShaderStage::kVertex;

        ShaderModuleDesc() = default;

        ShaderModuleDesc(ShaderStage stage, festd::span<const uint8_t> byteCode)
            : m_byteCode(byteCode)
            , m_entryPoint("main")
            , m_stage(stage)
        {
        }
    };


    struct ShaderModule : public DeviceObject
    {
        FE_RTTI_Class(ShaderModule, "0040A2EF-9D25-42AC-9A95-B3F8D4288E49");

        ~ShaderModule() override = default;

        virtual ResultCode Init(const ShaderModuleDesc& desc) = 0;

        [[nodiscard]] virtual const ShaderModuleDesc& GetDesc() const = 0;
        [[nodiscard]] virtual ShaderReflection* GetReflection() = 0;
    };
} // namespace FE::Graphics::RHI
