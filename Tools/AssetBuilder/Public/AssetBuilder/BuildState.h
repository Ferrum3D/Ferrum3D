#pragma once
#include <Core/IO/Assets.h>
#include <Core/IO/Path.h>

namespace FE::AssetBuilder
{
    //! @brief Result of the most recent successful build of one product.
    //!
    //! The build state lives in the output directory under .ferrum-build/state, keyed by AssetID, so that builds never write to
    //! authored .asset files. It is a local cache: a missing state only costs a rebuild, and identical inputs produce the same
    //! artifact again.
    struct ProductBuildState final
    {
        FE_RTTI_Reflect("FF9FDE17-6C37-4328-93BD-91D15C024918");
        FE_RTTI_Serialize();

        IO::BuildKey m_buildKey = IO::BuildKey::kNull;
        IO::ArtifactID m_artifactId = IO::ArtifactID::kNull;
    };


    //! @brief Directory for build bookkeeping inside an output directory: staging, build state and server state.
    IO::Path GetBuildDirectory(const IO::Path& outputDirectory);

    //! @brief Load the state of a product. Returns false if the product has no readable state.
    bool LoadBuildState(const IO::Path& outputDirectory, IO::AssetID assetId, ProductBuildState& state);

    //! @brief Record the state of a product, replacing the previous one atomically.
    bool StoreBuildState(const IO::Path& outputDirectory, IO::AssetID assetId, const ProductBuildState& state);

    //! @brief Forget the state of a removed product.
    void RemoveBuildState(const IO::Path& outputDirectory, IO::AssetID assetId);

    //! @brief Write a file through a temporary file and an atomic replace.
    bool WriteFileAtomically(const IO::Path& path, festd::span<const std::byte> bytes);

    //! @brief Delete a directory with every file and directory inside it.
    void RemoveDirectoryTree(const IO::Path& directory);
} // namespace FE::AssetBuilder
