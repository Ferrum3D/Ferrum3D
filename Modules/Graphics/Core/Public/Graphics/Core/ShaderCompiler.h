#pragma once
#include <Core/Containers/ByteBuffer.h>
#include <Core/Memory/Memory.h>
#include <Graphics/Core/ShaderLibrary.h>
#include <Graphics/Core/ShaderStage.h>

namespace FE::Graphics::Core
{
    enum class ShaderCompilerConfig : uint32_t
    {
        //! kDebug in debug builds of the engine, kOptimized otherwise.
        kDefault,

        //! Unoptimized shaders with debug information.
        kDebug,

        //! Optimized shaders, as shipped.
        kOptimized,
    };


    //! @brief Resolve kDefault to the configuration that matches the build of the engine.
    ShaderCompilerConfig ResolveShaderCompilerConfig(ShaderCompilerConfig config);


    struct ShaderCompilerArgs final
    {
        std::pmr::memory_resource* m_binaryAllocator = nullptr;
        ShaderStage m_stage = ShaderStage::kUndefined;
        festd::span<const ShaderDefine> m_defines;
        Env::Name m_shaderName;
        ShaderCompilerConfig m_config = ShaderCompilerConfig::kDefault;
    };


    //! @brief Identify a compilation request independently of the compiler, so that a result compiled ahead of time can be
    //! found without loading the compiler.
    uint64_t ComputeShaderRequestKey(const ShaderCompilerArgs& args);


    struct ShaderCompilerResult final
    {
        ByteBuffer m_byteCode;       //!< Shader bytecode, DWORD aligned.
        uint64_t m_hash = 0;         //!< Shader hash.
        uint32_t m_byteCodeSize = 0; //!< Unaligned shader bytecode size in bytes.
        bool m_codeValid = false;    //!< True if shader was successfully compiled.
        bool m_hashValid = false;    //!< True if hash was successfully calculated.

        //! Virtual paths of the source files the result was compiled from. Filled for failed compilations too, so a fix in
        //! any of them can trigger a recompile.
        festd::inline_vector<Env::Name, 16> m_dependencies;

        //! Content hashes of the dependencies, in the same order.
        festd::inline_vector<uint64_t, 16> m_dependencyHashes;
    };


    struct ShaderCompiler : public Memory::RefCountedObjectBase
    {
        FE_RTTI("F3D5E284-1DBF-40CC-9790-7D97FA69B18D");

        ~ShaderCompiler() override = default;

        virtual ShaderCompilerResult CompileShader(const ShaderCompilerArgs& args) = 0;
    };


    //! @brief Create the shader compiler of the platform, e.g. for tools that compile shaders without a device.
    //!
    //! Results go through the persistent shader cache next to the executable, like those of the compiler a device uses.
    Rc<ShaderCompiler> CreateShaderCompiler();
} // namespace FE::Graphics::Core
