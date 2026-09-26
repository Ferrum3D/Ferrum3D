#include <AssetBuilder/ArtifactWriter.h>
#include <AssetBuilder/AssetFile.h>
#include <AssetBuilder/AssetPipeline.h>
#include <AssetBuilder/PipelineInternal.h>

#include <Core/IO/FileStream.h>
#include <Core/IO/MemoryStream.h>
#include <Core/Serialization/BinarySerialization.h>
#include <Core/Serialization/JsonSerialization.h>
#include <Graphics/Assets/Assets.h>
#include <Graphics/Assets/MaterialAssets.h>
#include <festd/unordered_map.h>

namespace FE::AssetBuilder
{
    using namespace Graphics;
    using namespace Internal;

    namespace
    {
        bool SerializeBuildSettings(const AssetFileArtifact& artifact, festd::pmr::vector<std::byte>& result)
        {
            IO::WriteOnlyMemoryStream stream;
            Serialization::TaggedBinaryFormat format;
            Serialization::SerializationContext context(&stream, format);
            if (context.Store(artifact.m_buildSettings) != Serialization::ResultCode::kSuccess)
                return false;

            stream.DumpAll(result);
            return true;
        }


        Uuid MakeUuid(const uint64_t low, const uint64_t high)
        {
            alignas(16) uint64_t words[2] = { low, high };
            Uuid result = Uuid::LoadAligned(words);
            result.m_bytes[6] = (result.m_bytes[6] & 0x0f) | 0x40;
            result.m_bytes[8] = (result.m_bytes[8] & 0x3f) | 0x80;
            return result;
        }


        IO::BuildKey ComputeBuildKey(const AssetFileArtifact& artifact, const festd::span<const std::byte> sourceData,
                                     const festd::span<const std::byte> settingsData,
                                     const festd::span<const IO::ArtifactID> dependencyArtifacts)
        {
            constexpr festd::string_view domain = "FerrumBuild/v1";
            constexpr festd::string_view compilerVersion = "AssetCompiler/v2";
            constexpr festd::string_view platform = "pc";

            Hasher lowHasher(0x1fd79b78c365ade1ull);
            Hasher highHasher(0x86d2f9a3ab8c4075ull);
            lowHasher.Update(domain.data(), domain.size())
                .Update(artifact.m_assetId)
                .Update(artifact.m_assetTypeId)
                .Update(compilerVersion.data(), compilerVersion.size())
                .Update(platform.data(), platform.size())
                .Update(settingsData.data(), settingsData.size())
                .Update(sourceData.data(), sourceData.size());
            highHasher.Update(sourceData.data(), sourceData.size())
                .Update(settingsData.data(), settingsData.size())
                .Update(platform.data(), platform.size())
                .Update(compilerVersion.data(), compilerVersion.size())
                .Update(artifact.m_assetTypeId)
                .Update(artifact.m_assetId)
                .Update(domain.data(), domain.size());

            uint64_t assetSchemaHash = 0;
            if (artifact.m_buildSettings.GetType() != nullptr
                && artifact.m_buildSettings.GetType()->m_id == artifact.m_assetTypeId)
            {
                assetSchemaHash = artifact.m_buildSettings.GetType()->m_serializationSchemaHash;
            }
            else if (artifact.m_assetTypeId == Rtti::GetTypeID<MaterialAsset>())
            {
                assetSchemaHash = Serialization::GetSchemaHash<MaterialAsset>();
            }
            else if (artifact.m_assetTypeId == Rtti::GetTypeID<MaterialInstanceAsset>())
            {
                assetSchemaHash = Serialization::GetSchemaHash<MaterialInstanceAsset>();
            }

            if (assetSchemaHash != 0)
            {
                lowHasher.Update(assetSchemaHash);
                highHasher.Update(assetSchemaHash);
            }

            FE_Assert(dependencyArtifacts.size() == artifact.m_dependencies.size());
            for (uint32_t index = 0; index < dependencyArtifacts.size(); ++index)
            {
                const AssetFileDependency& dependency = artifact.m_dependencies[index];
                const IO::ArtifactID dependencyArtifact = dependencyArtifacts[index];
                lowHasher.Update(dependency.m_assetId).Update(dependency.m_expectedTypeId).Update(dependency.m_kind);
                lowHasher.Update(dependencyArtifact);
                highHasher.Update(dependency.m_kind).Update(dependency.m_expectedTypeId).Update(dependency.m_assetId);
                highHasher.Update(dependencyArtifact);
            }

            return MakeUuid(lowHasher.Finalize(), highHasher.Finalize());
        }


        bool ResolveExternalDependency(const IO::Path& outputDirectory, const AssetFileDependency& dependency,
                                       IO::ArtifactID& artifactId)
        {
            const IO::Path metadataPath = ArtifactWriter::GetMetadataPath(outputDirectory, dependency.m_assetId);
            auto file = IO::FileStream::Open(metadataPath, IO::OpenMode::kReadOnly);
            if (!file)
            {
                Logger::LogError("Missing built dependency {} at '{}'", dependency.m_assetId, metadataPath);
                return false;
            }

            IO::ArtifactRecord record;
            Serialization::JsonFormat format;
            Serialization::DeserializationContext context(file->Get(), format);
            if (context.Load(record) != Serialization::ResultCode::kSuccess || record.m_assetId != dependency.m_assetId
                || record.m_assetTypeId != dependency.m_expectedTypeId || !record.m_artifactId.IsValid())
            {
                Logger::LogError("Invalid built dependency {} at '{}'", dependency.m_assetId, metadataPath);
                return false;
            }

            artifactId = record.m_artifactId;
            return true;
        }


        struct BuildContext final
        {
            AssetFile& m_assetFile;
            IO::Path m_sourceRoot;
            IO::Path m_finalOutputDirectory;
            IO::Path m_stagingOutputDirectory;
            IO::Path m_primarySourcePath;
            festd::span<const std::byte> m_primarySourceData;
            festd::span<const std::byte> m_primaryLogicalInputData;
            festd::unordered_dense_map<IO::AssetID, uint32_t> m_artifactIndices;
            festd::unordered_dense_set<IO::AssetID> m_visiting;
            festd::unordered_dense_set<IO::AssetID> m_built;
            festd::inline_vector<uint32_t, 4> m_changedArtifactIndices;
            festd::inline_vector<IO::ArtifactID, 4> m_obsoleteArtifactIds;

            bool Build(const uint32_t artifactIndex)
            {
                AssetFileArtifact& artifact = m_assetFile.m_artifacts[artifactIndex];
                FE_Assert(!artifact.m_isRemoved);

                if (m_built.contains(artifact.m_assetId))
                    return true;

                if (m_visiting.contains(artifact.m_assetId))
                {
                    Logger::LogError("Artifact dependency cycle contains asset {}", artifact.m_assetId);
                    return false;
                }

                m_visiting.insert(artifact.m_assetId);
                const auto deferEraseVisiting = festd::defer([this, assetId = artifact.m_assetId] {
                    m_visiting.erase(assetId);
                });

                for (const AssetFileDependency& dependency : artifact.m_dependencies)
                {
                    const auto iter = m_artifactIndices.find(dependency.m_assetId);
                    if (iter == m_artifactIndices.end())
                    {
                        IO::ArtifactID externalId;
                        if (!ResolveExternalDependency(m_finalOutputDirectory, dependency, externalId))
                            return false;
                        continue;
                    }

                    const AssetFileArtifact& dependencyArtifact = m_assetFile.m_artifacts[iter->second];
                    if (dependencyArtifact.m_isRemoved)
                    {
                        Logger::LogError("Asset {} requires removed product {}", artifact.m_assetId, dependency.m_assetId);
                        return false;
                    }

                    if (dependencyArtifact.m_assetTypeId != dependency.m_expectedTypeId)
                    {
                        Logger::LogError("Asset {} dependency {} has an incompatible type",
                                         artifact.m_assetId,
                                         dependency.m_assetId);
                        return false;
                    }

                    if (!Build(iter->second))
                        return false;
                }

                BuildInputs inputs;
                if (!PrepareBuildInputs(artifact,
                                        m_sourceRoot,
                                        m_primarySourcePath,
                                        m_primarySourceData,
                                        m_primaryLogicalInputData,
                                        inputs))
                    return false;

                festd::pmr::vector<std::byte> settingsData;
                if (!SerializeBuildSettings(artifact, settingsData))
                    return false;

                festd::inline_vector<IO::ArtifactID, 4> dependencyArtifactIds;
                for (const AssetFileDependency& dependency : artifact.m_dependencies)
                {
                    const auto iter = m_artifactIndices.find(dependency.m_assetId);
                    if (iter != m_artifactIndices.end())
                    {
                        dependencyArtifactIds.push_back(m_assetFile.m_artifacts[iter->second].m_builtArtifactId);
                    }
                    else
                    {
                        IO::ArtifactID externalId;
                        if (!ResolveExternalDependency(m_finalOutputDirectory, dependency, externalId))
                            return false;
                        dependencyArtifactIds.push_back(externalId);
                    }
                }

                const IO::BuildKey buildKey =
                    ComputeBuildKey(artifact, inputs.m_logicalInputData, settingsData, dependencyArtifactIds);

                const bool dataExists = artifact.m_builtArtifactId.IsValid()
                    && IO::File::Exists(ArtifactWriter::GetDataPath(m_finalOutputDirectory, artifact.m_builtArtifactId));
                const bool metadataExists =
                    IO::File::Exists(ArtifactWriter::GetMetadataPath(m_finalOutputDirectory, artifact.m_assetId));
                if (artifact.m_lastBuildKey == buildKey && artifact.m_builtArtifactId.IsValid() && dataExists && metadataExists)
                {
                    m_built.insert(artifact.m_assetId);
                    return true;
                }

                const BuildFunction builder = FindBuilder(artifact.m_assetTypeId);
                if (builder == nullptr)
                {
                    Logger::LogError("Product '{}' has no builder for type {}", artifact.m_name, artifact.m_assetTypeId);
                    return false;
                }

                const IO::ArtifactID previousArtifactId = artifact.m_builtArtifactId;
                IO::ArtifactID artifactId = IO::ArtifactID::kNull;
                const BuildRequest request{ artifact,
                                            inputs.m_sourcePath,
                                            inputs.m_sourceData,
                                            m_stagingOutputDirectory,
                                            &artifactId };
                if (!builder(request))
                    return false;

                if (!artifactId.IsValid())
                {
                    Logger::LogError("Builder produced no artifact identity for product '{}'", artifact.m_name);
                    return false;
                }

                artifact.m_lastBuildKey = buildKey;
                artifact.m_builtArtifactId = artifactId;
                m_changedArtifactIndices.push_back(artifactIndex);
                if (previousArtifactId.IsValid() && previousArtifactId != artifactId)
                    m_obsoleteArtifactIds.push_back(previousArtifactId);
                m_built.insert(artifact.m_assetId);
                return true;
            }
        };


        struct MetadataPublication final
        {
            IO::Path m_candidatePath;
            IO::Path m_finalPath;
            IO::Path m_backupPath;
            bool m_isRemoval = false;
            bool m_hadPrevious = false;
            bool m_candidatePublished = false;
        };


        void RollbackMetadata(festd::span<MetadataPublication> publications)
        {
            for (uint32_t index = publications.size(); index > 0; --index)
            {
                MetadataPublication& publication = publications[index - 1];
                if (publication.m_candidatePublished)
                    IO::File::Delete(publication.m_finalPath);
                if (publication.m_hadPrevious)
                    IO::File::Replace(publication.m_backupPath, publication.m_finalPath);
            }
        }


        bool PublishBuild(BuildContext& context, const Uuid transactionId)
        {
            for (const uint32_t artifactIndex : context.m_changedArtifactIndices)
            {
                const AssetFileArtifact& artifact = context.m_assetFile.m_artifacts[artifactIndex];
                const IO::Path candidatePath =
                    ArtifactWriter::GetDataPath(context.m_stagingOutputDirectory, artifact.m_builtArtifactId);
                const IO::Path finalPath =
                    ArtifactWriter::GetDataPath(context.m_finalOutputDirectory, artifact.m_builtArtifactId);
                const IO::ResultCode createResult = IO::Directory::Create(IO::PathView(finalPath).parent_directory());

                if (createResult != IO::ResultCode::kSuccess)
                {
                    Logger::LogError("Failed to create artifact publication directory '{}': {}",
                                     finalPath,
                                     IO::GetResultDesc(createResult));
                    return false;
                }

                if (IO::File::Exists(finalPath))
                {
                    if (IO::File::Delete(candidatePath) != IO::ResultCode::kSuccess)
                        return false;
                    continue;
                }

                const IO::ResultCode moveResult = IO::File::Move(candidatePath, finalPath);
                if (moveResult != IO::ResultCode::kSuccess)
                {
                    if (IO::File::Exists(finalPath))
                    {
                        if (IO::File::Delete(candidatePath) != IO::ResultCode::kSuccess)
                            return false;
                        continue;
                    }

                    Logger::LogError("Failed to publish immutable artifact '{}': {}", finalPath, IO::GetResultDesc(moveResult));
                    return false;
                }
            }

            festd::inline_vector<MetadataPublication, 4> publications;
            for (const uint32_t artifactIndex : context.m_changedArtifactIndices)
            {
                const AssetFileArtifact& artifact = context.m_assetFile.m_artifacts[artifactIndex];
                MetadataPublication& publication = publications.emplace_back();
                publication.m_candidatePath =
                    ArtifactWriter::GetMetadataPath(context.m_stagingOutputDirectory, artifact.m_assetId);
                publication.m_finalPath = ArtifactWriter::GetMetadataPath(context.m_finalOutputDirectory, artifact.m_assetId);
            }

            for (const AssetFileArtifact& artifact : context.m_assetFile.m_artifacts)
            {
                if (!artifact.m_isRemoved)
                    continue;

                MetadataPublication& publication = publications.emplace_back();
                publication.m_finalPath = ArtifactWriter::GetMetadataPath(context.m_finalOutputDirectory, artifact.m_assetId);
                publication.m_isRemoval = true;
            }

            for (uint32_t index = 0; index < publications.size(); ++index)
            {
                MetadataPublication& publication = publications[index];
                const IO::ResultCode createResult =
                    IO::Directory::Create(IO::PathView(publication.m_finalPath).parent_directory());
                if (createResult != IO::ResultCode::kSuccess)
                {
                    RollbackMetadata(festd::span(publications.data(), index));
                    return false;
                }

                publication.m_backupPath = publication.m_finalPath;
                publication.m_backupPath.AsBaseString() += Fmt::FixedFormat(".{}.backup", transactionId);
                publication.m_hadPrevious = IO::File::Exists(publication.m_finalPath);
                if (publication.m_hadPrevious)
                {
                    const IO::ResultCode backupResult = IO::File::Move(publication.m_finalPath, publication.m_backupPath);
                    if (backupResult != IO::ResultCode::kSuccess)
                    {
                        Logger::LogError("Failed to back up artifact metadata '{}': {}",
                                         publication.m_finalPath,
                                         IO::GetResultDesc(backupResult));
                        RollbackMetadata(festd::span(publications.data(), index));
                        return false;
                    }
                }

                if (!publication.m_isRemoval)
                {
                    const IO::ResultCode publishResult = IO::File::Move(publication.m_candidatePath, publication.m_finalPath);
                    if (publishResult != IO::ResultCode::kSuccess)
                    {
                        Logger::LogError("Failed to publish artifact metadata '{}': {}",
                                         publication.m_finalPath,
                                         IO::GetResultDesc(publishResult));
                        RollbackMetadata(festd::span(publications.data(), index + 1));
                        return false;
                    }

                    publication.m_candidatePublished = true;
                }
            }

            for (MetadataPublication& publication : publications)
            {
                if (publication.m_hadPrevious)
                    IO::File::Delete(publication.m_backupPath);
            }

            return true;
        }


        void RemoveEmptyStagingAncestors(const IO::Path& filePath, const IO::Path& stagingDirectory)
        {
            IO::Path directory = IO::NormalizePath(IO::PathView(filePath).parent_directory());
            while (directory.size() >= stagingDirectory.size() && PathsHaveEqualPrefix(directory, stagingDirectory))
            {
                if (IO::Directory::Delete(directory) != IO::ResultCode::kSuccess)
                    break;
                if (directory == stagingDirectory)
                    break;
                directory = IO::NormalizePath(IO::PathView(directory).parent_directory());
            }
        }


        void RemoveStagingDirectory(const BuildContext& context)
        {
            for (const AssetFileArtifact& artifact : context.m_assetFile.m_artifacts)
            {
                const IO::Path pendingPath =
                    ArtifactWriter::GetPendingDataPath(context.m_stagingOutputDirectory, artifact.m_assetId);
                IO::File::Delete(pendingPath);
                RemoveEmptyStagingAncestors(pendingPath, context.m_stagingOutputDirectory);

                if (artifact.m_builtArtifactId.IsValid())
                {
                    const IO::Path dataPath =
                        ArtifactWriter::GetDataPath(context.m_stagingOutputDirectory, artifact.m_builtArtifactId);
                    IO::File::Delete(dataPath);
                    RemoveEmptyStagingAncestors(dataPath, context.m_stagingOutputDirectory);
                }

                const IO::Path metadataPath =
                    ArtifactWriter::GetMetadataPath(context.m_stagingOutputDirectory, artifact.m_assetId);
                IO::File::Delete(metadataPath);
                RemoveEmptyStagingAncestors(metadataPath, context.m_stagingOutputDirectory);
            }
            IO::Directory::Delete(context.m_stagingOutputDirectory);
        }
    } // namespace


    bool BuildAsset(const BuildAssetSettings& settings)
    {
        if (settings.m_outputDirectory.empty())
        {
            Logger::LogError("An artifact output directory is required to build an asset");
            return false;
        }

        const IO::Path assetFilePath = IO::GetAbsolutePath(settings.m_assetFile);
        IO::Path sourceRoot;
        if (!settings.m_sourceRoot.empty())
            sourceRoot = IO::GetAbsolutePath(settings.m_sourceRoot);
        const IO::Path outputDirectory = IO::GetAbsolutePath(settings.m_outputDirectory);
        if (IO::PathView(assetFilePath).extension() != ".asset")
        {
            Logger::LogError("Build input '{}' is not an .asset file", assetFilePath);
            return false;
        }

        AssetFile assetFile;
        if (!LoadAssetFile(assetFilePath, assetFile))
        {
            Logger::LogError("Failed to read asset file '{}'", assetFilePath);
            return false;
        }

        if (!assetFile.m_sourcePath.empty())
        {
            if (sourceRoot.empty())
            {
                Logger::LogError("A source depot root is required to build file-backed asset '{}'", assetFilePath);
                return false;
            }

            if (!IO::File::Exists(sourceRoot / kSourceDepotRootMarker))
            {
                Logger::LogError("Source root '{}' does not contain '{}'", sourceRoot, kSourceDepotRootMarker);
                return false;
            }
        }

        if (assetFile.m_artifacts.empty())
        {
            Logger::LogError("Asset file '{}' declares no artifact products", assetFilePath);
            return false;
        }

        if (assetFile.m_sourcePath.empty())
        {
            for (AssetFileArtifact& artifact : assetFile.m_artifacts)
            {
                if (!artifact.m_isRemoved && artifact.m_buildSettings.GetType() != nullptr
                    && artifact.m_buildSettings.GetType()->m_id == artifact.m_assetTypeId
                    && !RefreshSerializedAssetDependencies(artifact))
                {
                    return false;
                }
            }
        }

        festd::vector<std::byte> sourceBytes;
        festd::vector<std::byte> logicalInputBytes;
        IO::Path sourcePath;
        if (assetFile.m_sourcePath.empty())
        {
            sourcePath = assetFilePath;
        }
        else
        {
            if (!ResolveSourceReference(sourceRoot, assetFile.m_sourcePath, sourcePath))
                return false;

            if (!ReadFile(sourcePath, sourceBytes))
            {
                Logger::LogError("Failed to read source '{}'", sourcePath);
                return false;
            }

            if (!AppendLogicalInput(logicalInputBytes, sourceBytes))
                return false;

            for (const IO::Path& dependencyReference : assetFile.m_sourceDependencies)
            {
                IO::Path dependencyPath;
                if (!ResolveSourceReference(sourceRoot, dependencyReference, dependencyPath))
                    return false;

                festd::vector<std::byte> dependencyBytes;
                if (!ReadFile(dependencyPath, dependencyBytes))
                {
                    Logger::LogError("Failed to read source dependency '{}'", dependencyPath);
                    return false;
                }

                if (!AppendLogicalInput(logicalInputBytes, dependencyBytes))
                {
                    Logger::LogError("Logical inputs for '{}' exceed the supported size", assetFilePath);
                    return false;
                }
            }
        }

        const Uuid transactionId = GenerateAssetId();
        if (!transactionId.IsValid())
        {
            Logger::LogError("Failed to generate an asset build transaction ID");
            return false;
        }

        IO::Path stagingDirectory = outputDirectory / ".ferrum-build";
        stagingDirectory /= Fmt::FixedFormat("{}", transactionId);
        BuildContext context{
            assetFile, sourceRoot, outputDirectory, stagingDirectory, sourcePath, sourceBytes, logicalInputBytes
        };

        const auto deferRemoveStaging = festd::defer([&context] {
            RemoveStagingDirectory(context);
        });

        for (uint32_t index = 0; index < assetFile.m_artifacts.size(); ++index)
        {
            const AssetFileArtifact& artifact = assetFile.m_artifacts[index];
            if (artifact.m_productKey.empty() || !artifact.m_assetId.IsValid() || !artifact.m_assetTypeId.IsValid())
            {
                Logger::LogError("Product '{}' has invalid identity", artifact.m_name);
                return false;
            }

            if (!context.m_artifactIndices.emplace(artifact.m_assetId, index).second)
            {
                Logger::LogError("Asset file '{}' contains duplicate asset ID {}", assetFilePath, artifact.m_assetId);
                return false;
            }

            for (uint32_t previousIndex = 0; previousIndex < index; ++previousIndex)
            {
                const AssetFileArtifact& previousArtifact = assetFile.m_artifacts[previousIndex];
                if (previousArtifact.m_productKey == artifact.m_productKey
                    && previousArtifact.m_assetTypeId == artifact.m_assetTypeId)
                {
                    Logger::LogError("Asset file '{}' contains duplicate product key '{}'", assetFilePath, artifact.m_productKey);
                    return false;
                }
            }
        }

        for (uint32_t index = 0; index < assetFile.m_artifacts.size(); ++index)
        {
            if (assetFile.m_artifacts[index].m_isRemoved)
                continue;
            if (!context.Build(index))
                return false;
        }

        if (!PublishBuild(context, transactionId))
            return false;

        for (AssetFileArtifact& artifact : assetFile.m_artifacts)
        {
            if (!artifact.m_isRemoved)
                continue;
            if (artifact.m_builtArtifactId.IsValid())
                context.m_obsoleteArtifactIds.push_back(artifact.m_builtArtifactId);
            artifact.m_lastBuildKey = IO::BuildKey::kNull;
            artifact.m_builtArtifactId = IO::ArtifactID::kNull;
        }

        if (!SaveAssetFile(assetFilePath, assetFile))
        {
            Logger::LogError("Failed to record build results in '{}'", assetFilePath);
            return false;
        }

        bool cleanupSucceeded = true;
        for (const IO::ArtifactID obsoleteArtifactId : context.m_obsoleteArtifactIds)
            cleanupSucceeded &= ArtifactWriter::RemoveArtifact(outputDirectory, obsoleteArtifactId);

        return cleanupSucceeded;
    }
} // namespace FE::AssetBuilder
