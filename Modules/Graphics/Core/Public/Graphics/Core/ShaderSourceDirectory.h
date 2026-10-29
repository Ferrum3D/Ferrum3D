#pragma once
#include <Core/IO/BaseIO.h>
#include <Core/Memory/RefCount.h>
#include <festd/string.h>
#include <festd/vector.h>

namespace FE::Graphics::Core
{
    //! @brief Make shader sources in an absolute directory visible to shader compilation.
    //!
    //! Shaders and includes are requested by virtual paths, e.g. "Shaders/Passes/Tools/Blit/Blit.ps.hlsl". A file at
    //! <directory>/<relative path> is visible as <virtualPrefix><relative path>. Registering the same pair again has no effect.
    //! Modules register their directories during initialization, before creating pipelines that use them.
    void AddShaderSourceDirectory(festd::string_view directory, festd::string_view virtualPrefix = {});

    //! @brief True if every registered shader source directory exists, e.g. in a development checkout.
    [[nodiscard]] bool AreShaderSourceDirectoriesAvailable();


    //! @brief Shader sources served from memory, e.g. from a published shader package asset.
    struct ShaderSourcePackage : public Memory::RefCountedObjectBase
    {
        //! @brief Copy the file visible at a virtual path. Returns false if the package has no such file.
        virtual bool ReadFile(festd::string_view virtualPath, festd::pmr::vector<std::byte>& contents) const = 0;
    };


    //! @brief Register the shader package asset that holds the same files as a module's shader source directory.
    //!
    //! Applications load the registered packages when sources have to come from assets, see AreShaderSourceDirectoriesAvailable.
    void AddShaderPackage(IO::AssetID packageId);

    //! @brief Append the registered shader package assets.
    void GetShaderPackages(festd::vector<IO::AssetID>& packageIds);

    //! @brief Make the files of a package visible, replacing the package previously mounted under the same key.
    //!
    //! A file in a registered directory takes precedence over a package file with the same virtual path, unless directories are
    //! disabled. Mounting reports every cached source as possibly changed, so that shader hot reload picks up new contents.
    void MountShaderSourcePackage(IO::AssetID key, ShaderSourcePackage* package);

    //! @brief Remove a package, unless another package has replaced it under the same key.
    void UnmountShaderSourcePackage(IO::AssetID key, const ShaderSourcePackage* package);

    //! @brief Read shader sources from registered directories, or only from mounted packages, as a shipping build does.
    void SetShaderSourceDirectoriesEnabled(bool enabled);
} // namespace FE::Graphics::Core
