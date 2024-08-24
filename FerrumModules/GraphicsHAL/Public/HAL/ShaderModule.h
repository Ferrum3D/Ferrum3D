#pragma once
#include <FeCore/Console/FeLog.h>
#include <FeCore/Containers/ByteBuffer.h>
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
        ShaderStage Stage = ShaderStage::Vertex;

        inline ShaderModuleDesc() = default;

        inline ShaderModuleDesc(ShaderStage stage, const ByteBuffer& byteCode)
            : ByteCode(byteCode.Data(), byteCode.Size())
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
