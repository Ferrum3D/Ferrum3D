#pragma once
#include <Core/IO/Path.h>
#include <Graphics/Core/ShaderCompiler.h>

namespace FE::Graphics::Core
{
    struct ShaderPermutationDefine final
    {
        festd::string m_name;
        festd::string m_value;
    };


    //! @brief One shader compilation requested at runtime.
    struct ShaderPermutation final
    {
        Env::Name m_shaderName;
        ShaderStage m_stage = ShaderStage::kUndefined;
        festd::inline_vector<ShaderPermutationDefine, 4> m_defines;

        //! @brief Get compiler arguments that refer to this permutation. The result is valid while the permutation is alive.
        void GetCompilerArgs(ShaderCompilerArgs& args, festd::inline_vector<ShaderDefine, 4>& defines) const;
    };


    //! @brief Location of the manifest of shader permutations requested by the executables that use a catalog.
    //!
    //! Development builds append each permutation they compile, one per line: the stage, the virtual path and the defines, e.g.
    //! "cs Shaders/Passes/Culling/Cull.cs.hlsl LOD_COUNT=4". The asset server prewarms the shader cache from it, and
    //! `ferrum build-shader-cache` compiles it into the precompiled shaders that ship with the catalog.
    IO::Path GetShaderPermutationManifestPath(const IO::Path& catalogDirectory);

    //! @brief Read every permutation of a manifest, skipping malformed lines and duplicates.
    bool LoadShaderPermutationManifest(const IO::Path& path, festd::vector<ShaderPermutation>& permutations);

    //! @brief Append a permutation to the manifest of the current catalog unless it is recorded already.
    //!
    //! Does nothing in shipping builds or without a catalog. Thread-safe.
    void RecordShaderPermutation(const ShaderCompilerArgs& args);
} // namespace FE::Graphics::Core
