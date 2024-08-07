﻿#pragma once
#include <FeCore/Console/FeLog.h>
#include <OsGPU/Shader/ShaderStage.h>
#include <FeCore/Containers/ByteBuffer.h>
#include <FeCore/Containers/ArraySlice.h>

namespace FE::Osmium
{
    class IShaderReflection;

    struct ShaderModuleDesc
    {
        ArraySlice<UInt8> ByteCode;
        String EntryPoint;
        ShaderStage Stage = ShaderStage::Vertex;

        FE_STRUCT_RTTI(ShaderModuleDesc, "25C38D43-1D2D-4EA2-9708-54C23DD04507");

        inline ShaderModuleDesc() = default;

        inline ShaderModuleDesc(ShaderStage stage, const ByteBuffer& byteCode)
            : ByteCode(byteCode.Data(), byteCode.Size())
            , EntryPoint("main")
            , Stage(stage)
        {
        }
    };

    class IShaderModule : public Memory::RefCountedObjectBase
    {
    public:
        FE_CLASS_RTTI(IShaderModule, "0040A2EF-9D25-42AC-9A95-B3F8D4288E49");

        ~IShaderModule() override = default;

        [[nodiscard]] virtual const ShaderModuleDesc& GetDesc() const = 0;
        [[nodiscard]] virtual IShaderReflection* GetReflection() = 0;
    };
} // namespace FE
