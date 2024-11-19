#pragma once
#include <FeCore/Containers/ByteBuffer.h>
#include <FeCore/Memory/Memory.h>
#include <FeCore/Strings/String.h>
#include <Graphics/RHI/Common/BaseTypes.h>
#include <Graphics/RHI/ShaderStage.h>

namespace FE::Graphics::RHI
{
    struct HLSLShaderVersion final
    {
        uint32_t m_major = 6;
        uint32_t m_minor = 0;

        HLSLShaderVersion() = default;

        HLSLShaderVersion(uint32_t major, uint32_t minor)
            : m_major(major)
            , m_minor(minor)
        {
        }
    };


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
