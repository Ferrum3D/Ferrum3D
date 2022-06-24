#pragma once
#include <FeCore/Console/FeLog.h>
#include <FeCore/Memory/SharedPtr.h>
#include <OsGPU/Shader/ShaderStage.h>
#include <FeCore/Containers/IByteBuffer.h>

namespace FE::GPU
{
    class IShaderReflection;

    struct ShaderModuleDesc
    {
        const UInt8* ByteCode = nullptr;
        USize ByteCodeSize = 0;
        String EntryPoint;
        ShaderStage Stage = ShaderStage::Vertex;

        FE_STRUCT_RTTI(ShaderModuleDesc, "25C38D43-1D2D-4EA2-9708-54C23DD04507");

        inline ShaderModuleDesc() = default;

        inline ShaderModuleDesc(ShaderStage stage, const Shared<IByteBuffer>& byteCode)
            : ByteCode(byteCode->Data())
            , ByteCodeSize(byteCode->Size())
            , EntryPoint("main")
            , Stage(stage)
        {
        }
    };

    class IShaderModule : public IObject
    {
    public:
        FE_CLASS_RTTI(IShaderModule, "0040A2EF-9D25-42AC-9A95-B3F8D4288E49");

        ~IShaderModule() override = default;

        [[nodiscard]] virtual const ShaderModuleDesc& GetDesc() const = 0;
        [[nodiscard]] virtual IShaderReflection* GetReflection() = 0;
    };
} // namespace FE
