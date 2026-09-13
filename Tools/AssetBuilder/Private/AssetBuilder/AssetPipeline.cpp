#include <AssetBuilder/ArtifactWriter.h>
#include <AssetBuilder/AssetFile.h>
#include <AssetBuilder/AssetPipeline.h>
#include <AssetBuilder/ModelImporter.h>
#include <AssetBuilder/ModelProcessor.h>
#include <AssetBuilder/TextureProcessor.h>

#include <Core/Base/PlatformInclude.h>
#include <Core/IO/FileStream.h>
#include <Core/IO/StreamBase.h>
#include <Core/Serialization/BinarySerialization.h>
#include <Graphics/Assets/Assets.h>
#include <bcrypt.h>
#include <festd/unordered_map.h>

namespace FE::AssetBuilder
{
    using namespace Graphics;

    namespace
    {
        struct MemoryOutputStream final : public IO::BufferedStream
        {
            MemoryOutputStream()
                : BufferedStream(nullptr)
            {
            }

            ~MemoryOutputStream() override
            {
                FlushWrites();
            }

            [[nodiscard]] bool SeekAllowed() const override
            {
                return false;
            }

            [[nodiscard]] bool IsOpen() const override
            {
                return true;
            }

            IO::ResultCode Seek(intptr_t, IO::SeekMode) override
            {
                return IO::ResultCode::kInvalidSeek;
            }

            [[nodiscard]] uintptr_t Tell() const override
            {
                return m_data.size() + m_bufferPosition;
            }

            [[nodiscard]] size_t Length() const override
            {
                return m_data.size() + m_bufferPosition;
            }

            size_t ReadToBuffer(void*, size_t) override
            {
                return 0;
            }

            [[nodiscard]] festd::string_view GetName() override
            {
                return "AssetBuildFingerprint";
            }

            [[nodiscard]] IO::OpenMode GetOpenMode() const override
            {
                return IO::OpenMode::kWriteOnly;
            }

            void Close() override {}

            [[nodiscard]] festd::span<const std::byte> GetData()
            {
                FlushWrites();
                return m_data;
            }

        private:
            festd::vector<std::byte> m_data;

            void DoRelease() override
            {
                FE_Assert(false, "Stack-owned stream cannot be released");
            }

            size_t WriteImpl(const void* buffer, const size_t byteSize) override
            {
                const uint32_t offset = m_data.size();
                m_data.resize(offset + static_cast<uint32_t>(byteSize));
                memcpy(m_data.data() + offset, buffer, byteSize);
                return byteSize;
            }
        };


        bool ReadFile(const IO::Path& path, festd::vector<std::byte>& bytes)
        {
            auto fileResult = IO::FileStream::Open(path, IO::OpenMode::kReadOnly);
            if (!fileResult)
                return false;

            const size_t size = (*fileResult)->Length();
            if (size > Constants::kMaxU32)
                return false;

            bytes.resize(static_cast<uint32_t>(size));
            return (*fileResult)->ReadToBuffer(bytes.data(), bytes.size()) == size;
        }


        IO::AssetID GenerateAssetId()
        {
            IO::AssetID result{ kForceInit };
            const NTSTATUS status =
                BCryptGenRandom(nullptr, result.data(), static_cast<ULONG>(result.size()), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
            if (status < 0)
                return IO::AssetID::kNull;

            result.m_bytes[6] = (result.m_bytes[6] & 0x0f) | 0x40;
            result.m_bytes[8] = (result.m_bytes[8] & 0x3f) | 0x80;
            return result;
        }


        const AssetFileArtifact* FindPreviousArtifact(const AssetFile* previous, const festd::string_view name,
                                                      const Rtti::TypeID typeId)
        {
            if (previous == nullptr)
                return nullptr;

            for (const AssetFileArtifact& artifact : previous->m_artifacts)
            {
                if (artifact.m_name == name && artifact.m_assetTypeId == typeId)
                    return &artifact;
            }
            return nullptr;
        }


        bool InitializeArtifact(AssetFileArtifact& artifact, const AssetFile* previous, const festd::string_view name,
                                const Rtti::TypeID typeId)
        {
            artifact.m_name = name;
            artifact.m_assetTypeId = typeId;
            if (const AssetFileArtifact* oldArtifact = FindPreviousArtifact(previous, name, typeId))
            {
                artifact.m_assetId = oldArtifact->m_assetId;
                artifact.m_buildSettings = oldArtifact->m_buildSettings;
            }
            else
            {
                artifact.m_assetId = GenerateAssetId();
            }

            if (artifact.m_assetId.IsValid())
                return true;

            Logger::LogError("Failed to generate an asset ID for product '{}'", name);
            return false;
        }


        void AddDependency(AssetFileArtifact& artifact, const AssetFileArtifact& dependency)
        {
            AssetFileDependency& result = artifact.m_dependencies.emplace_back();
            result.m_assetId = dependency.m_assetId;
            result.m_expectedTypeId = dependency.m_assetTypeId;
        }


        bool LoadPreviousAssetFile(const IO::Path& path, AssetFile& previous, const AssetFile*& previousPtr)
        {
            previousPtr = nullptr;
            if (!IO::File::Exists(path))
                return true;
            if (!LoadAssetFile(path, previous))
            {
                Logger::LogError("Failed to read existing asset file '{}'", path);
                return false;
            }

            previousPtr = &previous;
            return true;
        }


        bool ImportTexture(const IO::Path& sourcePath, const IO::Path& assetFilePath)
        {
            festd::vector<std::byte> sourceBytes;
            if (!ReadFile(sourcePath, sourceBytes) || !ValidateTextureSource(sourcePath, sourceBytes))
                return false;

            AssetFile previous;
            const AssetFile* previousPtr;
            if (!LoadPreviousAssetFile(assetFilePath, previous, previousPtr))
                return false;

            AssetFile assetFile;
            assetFile.m_sourcePath = sourcePath;
            if (previousPtr != nullptr)
                assetFile.m_builtArtifactIds = previousPtr->m_builtArtifactIds;
            AssetFileArtifact& texture = assetFile.m_artifacts.emplace_back();
            if (!InitializeArtifact(texture, previousPtr, "Texture", Rtti::GetTypeID<TextureAsset>()))
                return false;
            texture.m_buildSettings.Emplace<TextureBuildSettings>();

            if (!SaveAssetFile(assetFilePath, assetFile))
            {
                Logger::LogError("Failed to write asset file '{}'", assetFilePath);
                return false;
            }

            Logger::LogInfo("Imported texture '{}' as '{}'", sourcePath, assetFilePath);
            return true;
        }


        bool ImportModel(const IO::Path& sourcePath, const IO::Path& assetFilePath)
        {
            festd::vector<std::byte> sourceBytes;
            if (!ReadFile(sourcePath, sourceBytes))
            {
                Logger::LogError("Failed to read model '{}'", sourcePath);
                return false;
            }

            ModelImporter importer = ModelImporter::Create(sourceBytes.data(), sourceBytes.size(), sourcePath);
            if (!importer)
                return false;

            IntermediateScene* scene = importer.ParseScene();
            const auto deferDeleteScene = festd::defer([scene] {
                Memory::DefaultDelete(scene);
            });
            if (scene->m_models.empty())
            {
                Logger::LogError("Model '{}' contains no artifact products", sourcePath);
                return false;
            }

            AssetFile previous;
            const AssetFile* previousPtr;
            if (!LoadPreviousAssetFile(assetFilePath, previous, previousPtr))
                return false;

            AssetFile assetFile;
            assetFile.m_sourcePath = sourcePath;
            if (previousPtr != nullptr)
                assetFile.m_builtArtifactIds = previousPtr->m_builtArtifactIds;

            festd::inline_vector<uint32_t, 4> textureArtifactIndices;
            for (uint32_t textureIndex = 0; textureIndex < scene->m_texturePaths.size(); ++textureIndex)
            {
                festd::vector<std::byte> textureBytes;
                const IO::Path& texturePath = scene->m_texturePaths[textureIndex];
                if (!ReadFile(texturePath, textureBytes) || !ValidateTextureSource(texturePath, textureBytes))
                    return false;

                const auto name = Fmt::FixedFormat("Texture.{}", textureIndex);
                AssetFileArtifact& texture = assetFile.m_artifacts.emplace_back();
                if (!InitializeArtifact(texture, previousPtr, name, Rtti::GetTypeID<TextureAsset>()))
                    return false;
                TextureBuildSettings textureSettings;
                textureSettings.m_sourceObjectIndex = textureIndex;
                textureSettings.m_sourcePath = texturePath;
                texture.m_buildSettings.Emplace<TextureBuildSettings>(std::move(textureSettings));
                textureArtifactIndices.push_back(assetFile.m_artifacts.size() - 1);
            }

            for (uint32_t modelIndex = 0; modelIndex < scene->m_models.size(); ++modelIndex)
            {
                const auto meshName = Fmt::FixedFormat("Mesh.{}", modelIndex);
                AssetFileArtifact& mesh = assetFile.m_artifacts.emplace_back();
                if (!InitializeArtifact(mesh, previousPtr, meshName, Rtti::GetTypeID<MeshAsset>()))
                    return false;
                MeshBuildSettings meshSettings;
                if (const auto* previousSettings = mesh.m_buildSettings.TryGet<MeshBuildSettings>())
                    meshSettings = *previousSettings;
                meshSettings.m_sourceObjectIndex = modelIndex;
                mesh.m_buildSettings.Emplace<MeshBuildSettings>(meshSettings);
                const uint32_t meshArtifactIndex = assetFile.m_artifacts.size() - 1;

                const auto modelName = Fmt::FixedFormat("Model.{}", modelIndex);
                AssetFileArtifact& model = assetFile.m_artifacts.emplace_back();
                if (!InitializeArtifact(model, previousPtr, modelName, Rtti::GetTypeID<ModelAsset>()))
                    return false;
                ModelBuildSettings modelSettings;
                modelSettings.m_sourceObjectIndex = modelIndex;
                model.m_buildSettings.Emplace<ModelBuildSettings>(modelSettings);
                AddDependency(model, assetFile.m_artifacts[meshArtifactIndex]);
                for (const uint32_t textureArtifactIndex : textureArtifactIndices)
                    AddDependency(model, assetFile.m_artifacts[textureArtifactIndex]);
            }

            if (!SaveAssetFile(assetFilePath, assetFile))
            {
                Logger::LogError("Failed to write asset file '{}'", assetFilePath);
                return false;
            }

            Logger::LogInfo("Imported model '{}' as '{}' with {} artifact products",
                            sourcePath,
                            assetFilePath,
                            assetFile.m_artifacts.size());
            return true;
        }


        bool MakeBuildFingerprint(const AssetFileArtifact& artifact, festd::vector<std::byte>& result)
        {
            MemoryOutputStream stream;
            Serialization::TaggedBinaryFormat format;
            Serialization::SerializationContext context(&stream, format);
            if (context.Store(artifact) != Serialization::ResultCode::kSuccess)
                return false;

            const festd::span<const std::byte> data = stream.GetData();
            result.assign(data.begin(), data.end());
            return true;
        }


        struct BuildRequest final
        {
            const AssetFileArtifact& m_artifact;
            IO::Path m_sourcePath;
            festd::span<const std::byte> m_sourceData;
            IO::Path m_outputDirectory;
            IO::ArtifactID m_artifactId;
        };


        bool BuildModelArtifact(const BuildRequest& request)
        {
            if (request.m_artifact.m_buildSettings.TryGet<ModelBuildSettings>() == nullptr)
            {
                Logger::LogError("Model product '{}' has incompatible build settings", request.m_artifact.m_name);
                return false;
            }

            ModelProcessSettings settings;
            settings.m_outputDirectory = request.m_outputDirectory;
            settings.m_assetId = request.m_artifact.m_assetId;
            settings.m_artifactId = request.m_artifactId;
            settings.m_dependencies = request.m_artifact.m_dependencies;
            return ProcessModel(settings);
        }


        bool BuildMeshArtifact(const BuildRequest& request)
        {
            const auto* buildSettings = request.m_artifact.m_buildSettings.TryGet<MeshBuildSettings>();
            if (buildSettings == nullptr)
            {
                Logger::LogError("Mesh product '{}' has incompatible build settings", request.m_artifact.m_name);
                return false;
            }

            MeshProcessSettings settings;
            settings.m_inputFile = request.m_sourcePath;
            settings.m_outputDirectory = request.m_outputDirectory;
            settings.m_assetId = request.m_artifact.m_assetId;
            settings.m_artifactId = request.m_artifactId;
            settings.m_sourceObjectIndex = buildSettings->m_sourceObjectIndex;
            settings.m_generateLods = buildSettings->m_generateLods;
            settings.m_sourceData = request.m_sourceData;
            return ProcessMesh(settings);
        }


        bool BuildTextureArtifact(const BuildRequest& request)
        {
            if (request.m_artifact.m_buildSettings.TryGet<TextureBuildSettings>() == nullptr)
            {
                Logger::LogError("Texture product '{}' has incompatible build settings", request.m_artifact.m_name);
                return false;
            }

            TextureProcessSettings settings;
            settings.m_inputFile = request.m_sourcePath;
            settings.m_outputDirectory = request.m_outputDirectory;
            settings.m_assetId = request.m_artifact.m_assetId;
            settings.m_artifactId = request.m_artifactId;
            settings.m_sourceData = request.m_sourceData;
            return ProcessTexture(settings);
        }


        using BuildFunction = bool (*)(const BuildRequest& request);

        struct BuilderEntry final
        {
            Rtti::TypeID m_typeId;
            BuildFunction m_function;
        };


        BuildFunction FindBuilder(const Rtti::TypeID typeId)
        {
            const BuilderEntry builders[] = {
                { Rtti::GetTypeID<ModelAsset>(), BuildModelArtifact },
                { Rtti::GetTypeID<MeshAsset>(), BuildMeshArtifact },
                { Rtti::GetTypeID<TextureAsset>(), BuildTextureArtifact },
            };
            for (const BuilderEntry& builder : builders)
            {
                if (builder.m_typeId == typeId)
                    return builder.m_function;
            }
            return nullptr;
        }


        struct BuildContext final
        {
            AssetFile& m_assetFile;
            IO::Path m_outputDirectory;
            IO::Path m_primarySourcePath;
            festd::span<const std::byte> m_primarySourceData;
            festd::unordered_dense_map<IO::AssetID, uint32_t> m_artifactIndices;
            festd::unordered_dense_set<IO::AssetID> m_visiting;
            festd::unordered_dense_set<IO::AssetID> m_built;
            festd::inline_vector<IO::ArtifactID, 4> m_newArtifactIds;

            bool Build(const uint32_t artifactIndex)
            {
                AssetFileArtifact& artifact = m_assetFile.m_artifacts[artifactIndex];
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
                        Logger::LogError("Asset {} requires an undeclared artifact {}", artifact.m_assetId, dependency.m_assetId);
                        return false;
                    }

                    const AssetFileArtifact& dependencyArtifact = m_assetFile.m_artifacts[iter->second];
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

                IO::Path sourcePath = m_primarySourcePath;
                festd::span<const std::byte> sourceData = m_primarySourceData;
                festd::vector<std::byte> externalSourceData;
                if (const auto* textureSettings = artifact.m_buildSettings.TryGet<TextureBuildSettings>();
                    textureSettings != nullptr && !textureSettings->m_sourcePath.empty())
                {
                    sourcePath = textureSettings->m_sourcePath;
                    if (!ReadFile(sourcePath, externalSourceData))
                    {
                        Logger::LogError("Failed to read source '{}' for product '{}'", sourcePath, artifact.m_name);
                        return false;
                    }
                    sourceData = externalSourceData;
                }

                festd::vector<std::byte> fingerprint;
                if (!MakeBuildFingerprint(artifact, fingerprint))
                    return false;
                const IO::ArtifactID artifactId =
                    ArtifactWriter::MakeArtifactID(artifact.m_assetId, sourceData, fingerprint, "AssetCompiler/v1");

                const BuildFunction builder = FindBuilder(artifact.m_assetTypeId);
                if (builder == nullptr)
                {
                    Logger::LogError("Product '{}' has no builder for type {}", artifact.m_name, artifact.m_assetTypeId);
                    return false;
                }

                const BuildRequest request{ artifact, sourcePath, sourceData, m_outputDirectory, artifactId };
                if (!builder(request))
                    return false;

                m_newArtifactIds.push_back(artifactId);
                m_built.insert(artifact.m_assetId);
                return true;
            }
        };
    } // namespace


    bool ImportAsset(const ImportAssetSettings& settings)
    {
        const IO::Path sourcePath = IO::GetAbsolutePath(settings.m_inputFile);
        const IO::Path assetFilePath = IO::GetAbsolutePath(settings.m_outputFile);
        if (IO::PathView(assetFilePath).extension() != ".asset")
        {
            Logger::LogError("Import output '{}' is not an .asset file", assetFilePath);
            return false;
        }

        const IO::PathView sourceView(sourcePath);
        if (sourceView.extension() == ".gltf" || sourceView.extension() == ".glb")
            return ImportModel(sourcePath, assetFilePath);
        if (sourceView.extension() == ".dds")
            return ImportTexture(sourcePath, assetFilePath);

        Logger::LogError("Unsupported source extension '{}'. Expected .gltf, .glb, or .dds", sourceView.extension());
        return false;
    }


    bool BuildAsset(const BuildAssetSettings& settings)
    {
        const IO::Path assetFilePath = IO::GetAbsolutePath(settings.m_assetFile);
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
        if (!assetFile.m_sourcePath.empty() && !assetFile.m_embeddedSourceData.empty())
        {
            Logger::LogError("File-backed asset '{}' must not contain embedded source data", assetFilePath);
            return false;
        }
        if (assetFile.m_artifacts.empty())
        {
            Logger::LogError("Asset file '{}' declares no artifact products", assetFilePath);
            return false;
        }

        festd::vector<std::byte> sourceBytes;
        IO::Path sourcePath;
        if (assetFile.m_sourcePath.empty())
        {
            sourcePath = assetFilePath;
            sourceBytes = assetFile.m_embeddedSourceData;
        }
        else
        {
            sourcePath = assetFile.m_sourcePath;
            if (!ReadFile(sourcePath, sourceBytes))
            {
                Logger::LogError("Failed to read source '{}'", sourcePath);
                return false;
            }
        }

        BuildContext context{ assetFile, IO::GetAbsolutePath(settings.m_outputDirectory), sourcePath, sourceBytes };
        for (uint32_t index = 0; index < assetFile.m_artifacts.size(); ++index)
        {
            const AssetFileArtifact& artifact = assetFile.m_artifacts[index];
            if (!artifact.m_assetId.IsValid() || !artifact.m_assetTypeId.IsValid())
            {
                Logger::LogError("Product '{}' has invalid identity", artifact.m_name);
                return false;
            }
            if (!context.m_artifactIndices.emplace(artifact.m_assetId, index).second)
            {
                Logger::LogError("Asset file '{}' contains duplicate asset ID {}", assetFilePath, artifact.m_assetId);
                return false;
            }
        }

        for (uint32_t index = 0; index < assetFile.m_artifacts.size(); ++index)
        {
            if (!context.Build(index))
                return false;
        }
        const auto oldArtifactIds = std::move(assetFile.m_builtArtifactIds);
        assetFile.m_builtArtifactIds = context.m_newArtifactIds;
        if (!SaveAssetFile(assetFilePath, assetFile))
        {
            Logger::LogError("Failed to record build results in '{}'", assetFilePath);
            return false;
        }

        bool cleanupSucceeded = true;
        for (const IO::ArtifactID oldArtifactId : oldArtifactIds)
        {
            if (festd::find(context.m_newArtifactIds.begin(), context.m_newArtifactIds.end(), oldArtifactId)
                == context.m_newArtifactIds.end())
            {
                cleanupSucceeded &= ArtifactWriter::RemoveArtifact(context.m_outputDirectory, oldArtifactId);
            }
        }
        return cleanupSucceeded;
    }
} // namespace FE::AssetBuilder
