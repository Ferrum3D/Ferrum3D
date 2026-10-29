#include <AssetBuilder/AssetDepot.h>
#include <AssetBuilder/AssetPipeline.h>
#include <AssetBuilder/PipelineInternal.h>
#include <AssetBuilder/ShaderProcessor.h>
#include <AssetBuilder/TextureProcessor.h>
#include <Core/Jobs/Jobs.h>
#include <Core/Logging/Logger.h>
#include <Graphics/Assets/MaterialAssets.h>

namespace FE::AssetBuilder
{
    using namespace Internal;

    namespace
    {
        //! True if a normalized path is inside a normalized directory, compared case-insensitively.
        bool IsInsideDirectory(const festd::string_view path, const festd::string_view directory)
        {
            return path.size() > directory.size() && PathsHaveEqualPrefix(path, directory)
                && path.byte_at(directory.size()) == '/';
        }


        //! Find the source root of a depot: a marker inside the depot, or else the nearest marked ancestor. A depot without one
        //! is valid and can only contain source-less assets.
        bool FindDepotSourceRoot(const IO::Path& depotDirectory, IO::Path& result)
        {
            festd::vector<IO::Path> markedDirectories;
            IO::Directory::TraverseRecursively(depotDirectory, [&markedDirectories](const IO::DirectoryEntry& entry) {
                if (entry.m_path.filename() == kSourceDepotRootMarker)
                    markedDirectories.push_back(IO::NormalizePath(entry.m_path.parent_directory()));

                return true;
            });

            if (markedDirectories.size() > 1)
            {
                Logger::LogError("Depot '{}' contains several source roots marked by '{}'",
                                 depotDirectory,
                                 kSourceDepotRootMarker);
                return false;
            }

            if (!markedDirectories.empty())
            {
                result = std::move(markedDirectories.front());
                return true;
            }

            IO::Path directory = depotDirectory;
            for (;;)
            {
                if (IO::File::Exists(directory / kSourceDepotRootMarker))
                {
                    result = directory;
                    return true;
                }

                IO::Path parent = IO::NormalizePath(IO::PathView{ directory }.parent_directory());
                if (parent.empty() || parent == directory)
                    return true;

                directory = std::move(parent);
            }
        }


        void AppendUnique(festd::vector<uint32_t>& values, const uint32_t value)
        {
            if (festd::find_index(festd::span{ values.data(), values.size() }, value) == kInvalidIndex)
                values.push_back(value);
        }


        //! Shared state of one BuildDepotAssets call; jobs only touch the entries of their own asset and their dependencies.
        struct DepotBuild final
        {
            enum class Status : uint8_t
            {
                kPending,
                kSucceeded,
                kFailed,
                kSkipped,
            };

            const AssetDepot* m_depot = nullptr;
            festd::vector<uint32_t> m_order;
            festd::unordered_dense_map<uint32_t, uint32_t> m_slots;
            festd::vector<Status> m_statuses;
            festd::vector<festd::vector<IO::AssetID>> m_publishedProducts;
            festd::vector<Rc<WaitGroup>> m_completions;

            void Build(const uint32_t slot)
            {
                const DepotAsset& asset = m_depot->GetAssets()[m_order[slot]];
                for (const uint32_t dependency : asset.m_dependencies)
                {
                    const auto dependencySlot = m_slots.find(dependency);
                    if (dependencySlot != m_slots.end() && m_statuses[dependencySlot->second] != Status::kSucceeded)
                    {
                        m_statuses[slot] = Status::kSkipped;
                        return;
                    }
                }

                BuildAssetSettings settings;
                settings.m_assetFile = asset.m_assetFile;
                settings.m_sourceRoot = asset.m_sourceRoot;
                settings.m_outputDirectory = m_depot->GetSettings().m_outputDirectory;
                settings.m_publishedProducts = &m_publishedProducts[slot];
                if (!BuildAsset(settings))
                {
                    Logger::LogError("Failed to build '{}'", asset.m_assetFile);
                    m_statuses[slot] = Status::kFailed;
                    return;
                }

                if (!m_publishedProducts[slot].empty())
                    Logger::LogInfo("Built '{}'", asset.m_assetFile);

                m_statuses[slot] = Status::kSucceeded;
            }
        };


        //! Orders assets so that dependencies in the set come first. Assets in a cycle are reported and left out.
        struct TopologicalSort final
        {
            enum class Mark : uint8_t
            {
                kNone,
                kVisiting,
                kDone,
            };

            TopologicalSort(const AssetDepot& depot, const festd::span<const uint32_t> assets)
                : m_depot(depot)
            {
                for (const uint32_t asset : assets)
                    m_marks.insert({ asset, Mark::kNone });

                for (const uint32_t asset : assets)
                    Visit(asset);
            }

            bool Visit(const uint32_t asset)
            {
                Mark& mark = m_marks[asset];
                if (mark == Mark::kDone)
                    return festd::find_index(festd::span{ m_cyclic.data(), m_cyclic.size() }, asset) == kInvalidIndex;

                if (mark == Mark::kVisiting)
                    return false;

                mark = Mark::kVisiting;

                bool isAcyclic = true;
                for (const uint32_t dependency : m_depot.GetAssets()[asset].m_dependencies)
                {
                    if (m_marks.contains(dependency))
                        isAcyclic &= Visit(dependency);
                }

                m_marks[asset] = Mark::kDone;
                if (isAcyclic)
                    m_order.push_back(asset);
                else
                    m_cyclic.push_back(asset);

                return isAcyclic;
            }

            const AssetDepot& m_depot;
            festd::unordered_dense_map<uint32_t, Mark> m_marks;
            festd::vector<uint32_t> m_order;
            festd::vector<uint32_t> m_cyclic;
        };
    } // namespace


    uint64_t HashPathKey(const festd::string_view normalizedPath)
    {
        Hasher hasher;
        for (uint32_t index = 0; index < normalizedPath.size(); ++index)
        {
            const char character = ASCII::ToLower(static_cast<char>(normalizedPath.byte_at(index)));
            hasher.UpdateRaw(character);
        }

        return hasher.Finalize();
    }


    bool AssetDepot::Scan(const DepotSettings& settings)
    {
        FE_PROFILER_ZONE();

        m_settings = {};
        m_settings.m_outputDirectory = IO::NormalizePath(IO::GetAbsolutePath(settings.m_outputDirectory));
        if (!settings.m_sourceRoot.empty())
            m_settings.m_sourceRoot = IO::NormalizePath(IO::GetAbsolutePath(settings.m_sourceRoot));

        for (const IO::Path& directory : settings.m_depotDirectories)
            m_settings.m_depotDirectories.push_back(IO::NormalizePath(IO::GetAbsolutePath(directory)));

        m_assets.clear();
        m_watchedDirectories.clear();
        m_assetsByInputFile.clear();
        m_assetsByProduct.clear();

        for (const IO::Path& directory : m_settings.m_depotDirectories)
        {
            if (!ScanDepot(directory))
                return false;
        }

        LinkDependencies();
        WatchExternalInputs();
        return true;
    }


    void AssetDepot::WatchExternalInputs()
    {
        // Inputs outside every depot and source root, such as the material library, are watched through their directory.
        for (const DepotAsset& asset : m_assets)
        {
            for (const IO::Path& input : asset.m_inputFiles)
            {
                const festd::string_view inputView{ input.data(), input.size() };

                bool isWatched = false;
                for (const IO::Path& directory : m_watchedDirectories)
                    isWatched |= IsInsideDirectory(inputView, festd::string_view{ directory.data(), directory.size() });

                if (!isWatched)
                    m_watchedDirectories.push_back(IO::NormalizePath(IO::PathView{ input }.parent_directory()));
            }
        }
    }


    bool AssetDepot::ScanDepot(const IO::Path& depotDirectory)
    {
        if (!IO::Directory::Exists(depotDirectory))
        {
            Logger::LogError("Depot directory '{}' does not exist", depotDirectory);
            return false;
        }

        IO::Path sourceRoot = m_settings.m_sourceRoot;
        if (sourceRoot.empty() && !FindDepotSourceRoot(depotDirectory, sourceRoot))
            return false;

        m_watchedDirectories.push_back(depotDirectory);
        const festd::string_view sourceRootView{ sourceRoot.data(), sourceRoot.size() };
        const festd::string_view depotView{ depotDirectory.data(), depotDirectory.size() };
        const bool isSourceRootWatched = sourceRoot.empty() || IsInsideDirectory(sourceRootView, depotView)
            || festd::find_index(GetWatchedDirectories(), sourceRoot) != kInvalidIndex;
        if (!isSourceRootWatched)
            m_watchedDirectories.push_back(sourceRoot);

        festd::vector<IO::Path> definitions;
        IO::Directory::TraverseRecursively(depotDirectory, [&definitions](const IO::DirectoryEntry& entry) {
            if (!Bit::AnySet(entry.m_attributes, IO::FileAttributeFlags::kDirectory) && entry.m_path.extension() == ".asset")
                definitions.push_back(IO::NormalizePath(festd::string_view{ entry.m_path }));

            return true;
        });

        eastl::sort(definitions.begin(), definitions.end(), [](const IO::Path& lhs, const IO::Path& rhs) {
            return festd::string_view{ lhs.data(), lhs.size() } < festd::string_view{ rhs.data(), rhs.size() };
        });

        for (IO::Path& definition : definitions)
        {
            const uint32_t assetIndex = m_assets.size();
            DepotAsset& asset = m_assets.push_back();
            asset.m_assetFile = std::move(definition);
            asset.m_sourceRoot = sourceRoot;
            if (!LoadAssetFile(asset.m_assetFile, asset.m_definition))
            {
                Logger::LogError("Failed to read asset file '{}'", asset.m_assetFile);
                return false;
            }

            for (AssetFileArtifact& artifact : asset.m_definition.m_artifacts)
            {
                if (artifact.m_isRemoved)
                    continue;

                // Builds refresh the links of source-less assets from their values, so the index must see the same edges.
                if (asset.m_definition.m_sourcePath.empty())
                    RefreshSerializedAssetDependencies(artifact);

                const auto [iter, isInserted] = m_assetsByProduct.insert({ artifact.m_assetId, assetIndex });
                if (!isInserted)
                {
                    Logger::LogError("Asset {} is defined by both '{}' and '{}'",
                                     artifact.m_assetId,
                                     m_assets[iter->second].m_assetFile,
                                     asset.m_assetFile);
                    return false;
                }
            }

            CollectInputs(asset);

            AddInput(asset.m_assetFile, assetIndex);
            for (const IO::Path& input : asset.m_inputFiles)
                AddInput(input, assetIndex);
        }

        return true;
    }


    void AssetDepot::CollectInputs(DepotAsset& asset) const
    {
        const auto addSource = [&asset](const IO::Path& reference) {
            if (asset.m_sourceRoot.empty())
            {
                Logger::LogWarning("'{}' references source '{}' but its depot has no source root", asset.m_assetFile, reference);
                return;
            }

            IO::Path path;
            if (ResolveSourceReference(asset.m_sourceRoot, reference, path))
                asset.m_inputFiles.push_back(std::move(path));
        };

        const AssetFile& definition = asset.m_definition;
        if (!definition.m_sourcePath.empty())
            addSource(definition.m_sourcePath);

        for (const IO::Path& dependency : definition.m_sourceDependencies)
            addSource(dependency);

        bool usesMaterialLibrary = false;
        for (const AssetFileArtifact& artifact : definition.m_artifacts)
        {
            if (artifact.m_isRemoved)
                continue;

            const auto* textureSettings = artifact.m_buildSettings.TryGet<TextureBuildSettings>();
            if (textureSettings != nullptr && !textureSettings->m_sourcePath.empty())
                addSource(textureSettings->m_sourcePath);

            const auto* packageSettings = artifact.m_buildSettings.TryGet<ShaderPackageBuildSettings>();
            if (packageSettings != nullptr && !asset.m_sourceRoot.empty())
            {
                IO::Path packageRoot;
                if (ResolveSourceReference(asset.m_sourceRoot, packageSettings->m_rootDirectory, packageRoot))
                    asset.m_inputDirectories.push_back(std::move(packageRoot));
            }

            usesMaterialLibrary |= artifact.m_assetTypeId == Rtti::GetTypeID<Graphics::MaterialAsset>();
        }

        if (usesMaterialLibrary)
            AppendMaterialLibraryFiles(asset.m_inputFiles);
    }


    void AssetDepot::AddInput(const IO::Path& file, const uint32_t assetIndex)
    {
        auto& assets = m_assetsByInputFile[HashPathKey(festd::string_view{ file.data(), file.size() })];
        if (festd::find_index(festd::span{ assets.data(), assets.size() }, assetIndex) == kInvalidIndex)
            assets.push_back(assetIndex);
    }


    void AssetDepot::LinkDependencies()
    {
        for (uint32_t assetIndex = 0; assetIndex < m_assets.size(); ++assetIndex)
        {
            for (const AssetFileArtifact& artifact : m_assets[assetIndex].m_definition.m_artifacts)
            {
                if (artifact.m_isRemoved)
                    continue;

                for (const AssetFileDependency& dependency : artifact.m_dependencies)
                {
                    if (dependency.m_kind != IO::DependencyKind::kHard)
                        continue;

                    const auto owner = m_assetsByProduct.find(dependency.m_assetId);
                    if (owner == m_assetsByProduct.end() || owner->second == assetIndex)
                        continue;

                    AppendUnique(m_assets[assetIndex].m_dependencies, owner->second);
                    AppendUnique(m_assets[owner->second].m_dependents, assetIndex);
                }
            }
        }
    }


    bool AssetDepot::IsDefinitionPath(const festd::string_view normalizedPath) const
    {
        if (IO::PathView{ normalizedPath }.extension() != ".asset")
            return false;

        for (const IO::Path& directory : m_settings.m_depotDirectories)
        {
            if (IsInsideDirectory(normalizedPath, festd::string_view{ directory.data(), directory.size() }))
                return true;
        }

        return false;
    }


    void AssetDepot::FindAffectedAssets(const festd::span<const IO::Path> changedFiles, festd::vector<uint32_t>& result) const
    {
        for (const IO::Path& file : changedFiles)
        {
            const festd::string_view path{ file.data(), file.size() };
            const auto inputs = m_assetsByInputFile.find(HashPathKey(path));
            if (inputs != m_assetsByInputFile.end())
            {
                for (const uint32_t assetIndex : inputs->second)
                    AppendUnique(result, assetIndex);
            }

            for (uint32_t assetIndex = 0; assetIndex < m_assets.size(); ++assetIndex)
            {
                for (const IO::Path& directory : m_assets[assetIndex].m_inputDirectories)
                {
                    if (IsInsideDirectory(path, festd::string_view{ directory.data(), directory.size() }))
                        AppendUnique(result, assetIndex);
                }
            }
        }
    }


    void AssetDepot::AddDependents(festd::vector<uint32_t>& assets) const
    {
        for (uint32_t index = 0; index < assets.size(); ++index)
        {
            for (const uint32_t dependent : m_assets[assets[index]].m_dependents)
                AppendUnique(assets, dependent);
        }
    }


    void AssetDepot::GetAllAssets(festd::vector<uint32_t>& result) const
    {
        for (uint32_t assetIndex = 0; assetIndex < m_assets.size(); ++assetIndex)
            result.push_back(assetIndex);
    }


    DepotBuildResult BuildDepotAssets(const AssetDepot& depot, const festd::span<const uint32_t> assets)
    {
        FE_PROFILER_ZONE();

        DepotBuildResult result;

        DepotBuild build;
        build.m_depot = &depot;

        TopologicalSort sort{ depot, assets };
        build.m_order = std::move(sort.m_order);
        for (const uint32_t asset : sort.m_cyclic)
        {
            Logger::LogError("'{}' is part of a dependency cycle", depot.GetAssets()[asset].m_assetFile);
            result.m_failedAssets.push_back(asset);
            ++result.m_skippedCount;
        }

        const uint32_t count = build.m_order.size();
        build.m_statuses.resize(count, DepotBuild::Status::kPending);
        build.m_publishedProducts.resize(count);
        for (uint32_t slot = 0; slot < count; ++slot)
        {
            build.m_slots.insert({ build.m_order[slot], slot });
            build.m_completions.push_back(WaitGroup::Create());
        }

        // Each build waits for the builds of its dependencies, so independent assets run in parallel.
        for (uint32_t slot = 0; slot < count; ++slot)
        {
            festd::inline_vector<WaitGroup*, 8> prerequisites;
            for (const uint32_t dependency : depot.GetAssets()[build.m_order[slot]].m_dependencies)
            {
                const auto dependencySlot = build.m_slots.find(dependency);
                if (dependencySlot != build.m_slots.end())
                    prerequisites.push_back(build.m_completions[dependencySlot->second].Get());
            }

            DepotBuild* buildPtr = &build;
            Jobs::DispatchForeground(
                festd::span<WaitGroup* const>{ prerequisites.data(), prerequisites.size() },
                [buildPtr, slot] {
                    buildPtr->Build(slot);
                },
                build.m_completions[slot].Get());
        }

        WaitGroup::WaitAll(festd::span<const Rc<WaitGroup>>{ build.m_completions.data(), build.m_completions.size() });

        for (uint32_t slot = 0; slot < count; ++slot)
        {
            switch (build.m_statuses[slot])
            {
            case DepotBuild::Status::kSucceeded:
                ++result.m_succeededCount;
                break;
            case DepotBuild::Status::kFailed:
                ++result.m_failedCount;
                result.m_failedAssets.push_back(build.m_order[slot]);
                break;
            default:
                ++result.m_skippedCount;
                result.m_failedAssets.push_back(build.m_order[slot]);
                break;
            }

            for (const IO::AssetID product : build.m_publishedProducts[slot])
                result.m_publishedProducts.push_back(product);
        }

        return result;
    }
} // namespace FE::AssetBuilder
