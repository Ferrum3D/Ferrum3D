#pragma once
#include <Core/IO/Path.h>
#include <Graphics/Assets/ShaderAssets.h>
#include <Graphics/Core/ShaderPermutationManifest.h>

namespace FE::AssetBuilder
{
    struct AssetDepot;


    //! @brief Authored settings of a ShaderPackageAsset product.
    struct ShaderPackageBuildSettings final
    {
        FE_RTTI_Reflect("627FF973-A905-4114-9107-D3A068A79D21");
        FE_RTTI_Serialize();

        //! Package root, relative to the source depot root.
        IO::Path m_rootDirectory;

        //! Extensions of the files in the package, e.g. ".hlsl". Every file is included when empty.
        festd::vector<festd::string> m_extensions;

        //! How package files map to virtual paths; must match the module's shader source directory registrations.
        festd::vector<Graphics::ShaderPackageMount> m_mounts;
    };


    //! @brief Register the source directories of every shader package in a depot with this process's shader compilers.
    void AddDepotShaderSourceDirectories(const AssetDepot& depot);


    //! @brief Compile shader permutations from the sources of a depot's shader packages, in parallel.
    //!
    //! Results go through the persistent shader cache next to the executable, which is shared by every executable in that
    //! directory. When results is set, it receives every shader that compiled.
    //!
    //! @return False if any permutation failed to compile.
    bool CompileShaderPermutations(const AssetDepot& depot, festd::span<const Graphics::Core::ShaderPermutation> permutations,
                                   Graphics::Core::ShaderCompilerConfig config,
                                   festd::vector<Graphics::Core::PrecompiledShader>* results);


    //! @brief Compile the shader permutation manifest of the depot's output into its precompiled shader cache asset.
    bool BuildShaderCache(const AssetDepot& depot, Graphics::Core::ShaderCompilerConfig config);
} // namespace FE::AssetBuilder
