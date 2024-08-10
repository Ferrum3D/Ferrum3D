#pragma once
#include <FeCore/Containers/ByteBuffer.h>
#include <FeCore/Memory/Memory.h>
#include <FeCore/Strings/String.h>
#include <OsGPU/Common/BaseTypes.h>
#include <OsGPU/Shader/ShaderStage.h>

namespace FE::Osmium
{
    struct HLSLShaderVersion
    {
        uint32_t Major = 6;
        uint32_t Minor = 0;

        FE_RTTI_Base(HLSLShaderVersion, "68EBC362-A8FE-4473-ADE3-7D5D8E86EA30");

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
        ShaderStage Stage = ShaderStage::Vertex;
        StringSlice SourceCode;
        StringSlice EntryPoint;
        StringSlice FullPath;

        FE_RTTI_Base(ShaderCompilerArgs, "58A284CE-87C0-4142-AF5B-86539F015382");
    };

    class IShaderCompiler : public Memory::RefCountedObjectBase
    {
    public:
        FE_RTTI_Class(IShaderCompiler, "F3D5E284-1DBF-40CC-9790-7D97FA69B18D");

        ~IShaderCompiler() override = default;

        virtual ByteBuffer CompileShader(const ShaderCompilerArgs& args) = 0;
    };
} // namespace FE::Osmium
