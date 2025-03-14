#pragma once
#include <FeCore/Containers/ByteBuffer.h>
#include <FeCore/Memory/Memory.h>
#include <Graphics/Core/Base/BaseTypes.h>
#include <Graphics/Core/ShaderLibrary.h>
#include <Graphics/Core/ShaderStage.h>

namespace FE::Graphics::Core
{
    struct ShaderCompilerArgs final
    {
        std::pmr::memory_resource* m_binaryAllocator = nullptr;
        ShaderStage m_stage = ShaderStage::kUndefined;
        festd::span<const ShaderDefine> m_defines;
        Env::Name m_shaderName;
    };


    struct ShaderCompilerResult final
    {
        ByteBuffer m_byteCode;       //!< Shader bytecode, DWORD aligned.
        uint64_t m_hash = 0;         //!< Shader hash.
        uint32_t m_byteCodeSize = 0; //! Unaligned shader bytecode size in bytes.
        bool m_codeValid = false;    //!< True if shader was successfully compiled.
        bool m_hashValid = false;    //!< True if hash was successfully calculated.
    };


    struct ShaderCompiler : public Memory::RefCountedObjectBase
    {
        FE_RTTI_Class(ShaderCompiler, "F3D5E284-1DBF-40CC-9790-7D97FA69B18D");

        ~ShaderCompiler() override = default;

        virtual ShaderCompilerResult CompileShader(const ShaderCompilerArgs& args) = 0;
    };
} // namespace FE::Graphics::Core
