#pragma once
#include <FeCore/Containers/ByteBuffer.h>
#include <FeCore/Logging/Trace.h>
#include <FeCore/Strings/FixedString.h>
#include <HAL/DeviceObject.h>
#include <HAL/ShaderStage.h>

namespace FE::Graphics::HAL
{
    class ShaderReflection;

    struct ShaderModuleDesc
    {
        festd::span<const uint8_t> ByteCode;
        FixStr32 EntryPoint;
        ShaderStage Stage = ShaderStage::kVertex;

        inline ShaderModuleDesc() = default;

        inline ShaderModuleDesc(ShaderStage stage, festd::span<const uint8_t> byteCode)
            : ByteCode(byteCode)
            , EntryPoint("main")
            , Stage(stage)
        {
        }
    };


    class ShaderModule : public DeviceObject
    {
    public:
        FE_RTTI_Class(ShaderModule, "0040A2EF-9D25-42AC-9A95-B3F8D4288E49");

        ~ShaderModule() override = default;

        virtual ResultCode Init(const ShaderModuleDesc& desc) = 0;

        [[nodiscard]] virtual const ShaderModuleDesc& GetDesc() const = 0;
        [[nodiscard]] virtual ShaderReflection* GetReflection() = 0;
    };
} // namespace FE::Graphics::HAL
