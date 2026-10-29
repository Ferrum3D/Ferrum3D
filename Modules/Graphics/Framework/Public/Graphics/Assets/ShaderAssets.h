#pragma once
#include <Core/IO/Assets.h>
#include <Graphics/Core/PrecompiledShaders.h>
#include <Graphics/Core/ShaderSourceDirectory.h>
#include <festd/string.h>
#include <festd/vector.h>

namespace FE::Graphics
{
    //! @brief One shader source file of a package.
    struct ShaderPackageFile final
    {
        FE_RTTI_Reflect("71A32085-A491-4520-83D6-A0D0D9517766");
        FE_RTTI_Serialize();

        //! Path relative to the package root, '/' separated.
        festd::string m_path;
        festd::string m_contents;
    };


    //! @brief Makes the package files under a directory visible under a virtual prefix, like AddShaderSourceDirectory.
    struct ShaderPackageMount final
    {
        FE_RTTI_Reflect("A18664C9-A88D-4D4E-A34D-B54D1DCECD52");
        FE_RTTI_Serialize();

        //! Directory relative to the package root, empty or ending with '/'.
        festd::string m_directory;

        //! Virtual path prefix, empty or ending with '/'.
        festd::string m_virtualPrefix;
    };


    //! @brief The shader sources of one module or game, as an asset.
    //!
    //! Executables read shader sources from registered directories during development. A package carries the same files, so
    //! that a build without the source tree can still compile its shaders. Shaders keep referring to files by virtual path.
    struct ShaderPackageAsset final
    {
        FE_RTTI("17DBC832-13C2-404E-8DB8-112CDB1C3EBC");
        FE_RTTI_Reflect();
        FE_RTTI_Serialize();

        festd::vector<ShaderPackageMount> m_mounts;

        //! Files sorted by path.
        festd::vector<ShaderPackageFile> m_files;

        FE_SKIP_SERIALIZING Rc<Core::ShaderSourcePackage> m_runtime;
    };


    //! @brief Shaders compiled ahead of time for the permutations a project uses, built by `ferrum build-shader-cache`.
    struct ShaderCacheAsset final
    {
        FE_RTTI("4408C5FE-3A99-4F64-98ED-CCC25312FEEF");
        FE_RTTI_Reflect();
        FE_RTTI_Serialize();

        festd::vector<Core::PrecompiledShader> m_shaders;

        FE_SKIP_SERIALIZING Rc<Core::PrecompiledShaderSet> m_runtime;
    };


    //! @brief Well-known identity of the precompiled shader cache in a project's catalog.
    inline const IO::AssetID kShaderCacheAssetId{ "06996E4C-3E3D-4678-A1CA-6CF9C303FC38" };

    //! @brief Shader package of the Graphics module, matching its shader source directory.
    inline const IO::AssetID kGraphicsShaderPackageId{ "424D87C5-85EF-4B0E-8AC6-CA9AF59EB0ED" };
} // namespace FE::Graphics
