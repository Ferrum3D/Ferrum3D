#pragma once
#include <FeCore/Containers/ByteBuffer.h>
#include <FeCore/Memory/Memory.h>
#include <Graphics/RHI/Base/BaseTypes.h>
#include <Graphics/RHI/ShaderStage.h>

namespace FE::Graphics::RHI
{
    struct ShaderCompilerArgs final
    {
        HLSLShaderVersion m_version;
        ShaderStage m_stage = ShaderStage::kVertex;
        Env::Name m_entryPoint;
        StringSlice m_sourceCode;
        StringSlice m_fullPath;
    };


    struct ShaderCompiler : public Memory::RefCountedObjectBase
    {
        FE_RTTI_Class(ShaderCompiler, "F3D5E284-1DBF-40CC-9790-7D97FA69B18D");

        ~ShaderCompiler() override = default;

        virtual ByteBuffer CompileShader(const ShaderCompilerArgs& args) = 0;
    };
} // namespace FE::Graphics::RHI
