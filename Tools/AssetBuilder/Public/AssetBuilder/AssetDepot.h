#pragma once
#include <AssetBuilder/AssetFile.h>
#include <festd/unordered_map.h>

namespace FE::AssetBuilder
{
    //! @brief One .asset file of a depot and everything its build reads.
    struct DepotAsset final
    {
        //! Normalized absolute path of the definition.
        IO::Path m_assetFile;

        //! Root that the definition's source references are relative to, or empty for a depot without sources.
        IO::Path m_sourceRoot;

        AssetFile m_definition;

        //! Normalized absolute source files read by the build, besides the definition itself.
        festd::vector<IO::Path> m_inputFiles;

        //! Normalized absolute directories whose files are all build inputs, e.g. the root of a shader package.
        festd::vector<IO::Path> m_inputDirectories;

        //! Depot assets that produce hard dependencies of this asset's products, and must therefore be built first.
        festd::vector<uint32_t> m_dependencies;

        //! Depot assets with a hard dependency on a product of this asset.
        festd::vector<uint32_t> m_dependents;
    };


    struct DepotSettings final
    {
        //! Directories that are searched recursively for .asset files.
        festd::vector<IO::Path> m_depotDirectories;

        //! Source root for every depot. When empty, each depot uses the directory marked by .ferrum-source-depot-root inside
        //! it, or the nearest marked ancestor of the depot directory.
        IO::Path m_sourceRoot;

        //! Artifact output directory shared by every depot.
        IO::Path m_outputDirectory;
    };


    //! @brief Index of every asset definition in a set of source depots, with their build inputs and dependency edges.
    //!
    //! The index answers which definitions a changed file affects and in which order definitions must be built. It is
    //! rebuilt from scratch by Scan, which is cheap compared to building, so it never needs incremental updates.
    struct AssetDepot final
    {
        //! @brief Find and read every definition. Returns false if a depot or definition cannot be read.
        bool Scan(const DepotSettings& settings);

        [[nodiscard]] const DepotSettings& GetSettings() const
        {
            return m_settings;
        }

        [[nodiscard]] festd::span<const DepotAsset> GetAssets() const
        {
            return festd::span{ m_assets.data(), m_assets.size() };
        }

        //! @brief Directories whose changes can affect builds: the depots, their source roots and the directories of other inputs.
        [[nodiscard]] festd::span<const IO::Path> GetWatchedDirectories() const
        {
            return festd::span{ m_watchedDirectories.data(), m_watchedDirectories.size() };
        }

        //! @brief True if a changed file is, or was, a definition inside a depot, so that the index has to be scanned again.
        [[nodiscard]] bool IsDefinitionPath(festd::string_view normalizedPath) const;

        //! @brief Append the indices of assets whose definition or inputs include a changed file, without duplicates.
        void FindAffectedAssets(festd::span<const IO::Path> changedFiles, festd::vector<uint32_t>& result) const;

        //! @brief Extend a set of asset indices with every transitive dependent, without duplicates.
        void AddDependents(festd::vector<uint32_t>& assets) const;

        //! @brief Append the indices of every asset.
        void GetAllAssets(festd::vector<uint32_t>& result) const;

    private:
        bool ScanDepot(const IO::Path& depotDirectory);
        void CollectInputs(DepotAsset& asset) const;
        void AddInput(const IO::Path& file, uint32_t assetIndex);
        void LinkDependencies();
        void WatchExternalInputs();

        DepotSettings m_settings;
        festd::vector<DepotAsset> m_assets;
        festd::vector<IO::Path> m_watchedDirectories;
        festd::unordered_dense_map<uint64_t, festd::inline_vector<uint32_t, 2>> m_assetsByInputFile;
        festd::unordered_dense_map<IO::AssetID, uint32_t> m_assetsByProduct;
    };


    struct DepotBuildResult final
    {
        //! Assets whose build succeeded, whether anything was rebuilt or not.
        uint32_t m_succeededCount = 0;

        //! Assets whose build failed.
        uint32_t m_failedCount = 0;

        //! Assets that were not built because a dependency failed or the order contains a cycle.
        uint32_t m_skippedCount = 0;

        //! Products whose artifact was published by the build, in no particular order.
        festd::vector<IO::AssetID> m_publishedProducts;

        //! Indices of assets that failed or were skipped.
        festd::vector<uint32_t> m_failedAssets;

        [[nodiscard]] bool IsSuccessful() const
        {
            return m_failedCount == 0 && m_skippedCount == 0;
        }
    };


    //! @brief Build depot assets in dependency order, running independent builds in parallel.
    //!
    //! Dependencies outside the requested set must already be built. A failed build skips its dependents.
    DepotBuildResult BuildDepotAssets(const AssetDepot& depot, festd::span<const uint32_t> assets);


    //! @brief Make a key for case-insensitive comparison of normalized paths.
    uint64_t HashPathKey(festd::string_view normalizedPath);
} // namespace FE::AssetBuilder
