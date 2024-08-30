#pragma once
#include <FeCore/Containers/ByteBuffer.h>
#include <FeCore/Memory/Memory.h>
#include <FeCore/Strings/String.h>
#include <HAL/Common/BaseTypes.h>
#include <HAL/ShaderStage.h>

namespace FE::Graphics::HAL
{
    struct HLSLShaderVersion
    {
        uint32_t Major = 6;
        uint32_t Minor = 0;

        HLSLShaderVersion() = default;

        inline HLSLShaderVersion(uint32_t major, uint32_t minor)
            : Major(major)
            , Minor(minor)
        {
        }
    };


    struct ShaderCompilerArgs
    {
        HLSLShaderVersion Version;
        ShaderStage Stage = ShaderStage::kVertex;
        StringSlice SourceCode;
        StringSlice EntryPoint;
        StringSlice FullPath;
    };


    class ShaderCompiler : public Memory::RefCountedObjectBase
    {
    public:
        FE_RTTI_Class(ShaderCompiler, "F3D5E284-1DBF-40CC-9790-7D97FA69B18D");

        ~ShaderCompiler() override = default;

        virtual ByteBuffer CompileShader(const ShaderCompilerArgs& args) = 0;
    };
} // namespace FE::Graphics::HAL
