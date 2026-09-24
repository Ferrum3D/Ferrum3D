#include <AssetBuilder/ArtifactWriter.h>
#include <AssetBuilder/AssetFile.h>
#include <AssetBuilder/AssetPipeline.h>
#include <AssetBuilder/MaterialProcessor.h>
#include <AssetBuilder/ModelImporter.h>
#include <AssetBuilder/ModelProcessor.h>
#include <AssetBuilder/TextureProcessor.h>

#include <Core/Base/PlatformInclude.h>
#include <Core/IO/FileStream.h>
#include <Core/IO/StreamBase.h>
#include <Core/Serialization/BinarySerialization.h>
#include <Core/Serialization/JsonSerialization.h>
#include <Graphics/Assets/Assets.h>
#include <Graphics/Assets/MaterialAssets.h>
#include <bcrypt.h>
#include <festd/unordered_map.h>

namespace FE::AssetBuilder
{
    using namespace Graphics;

    namespace
    {
        constexpr festd::string_view kSourceDepotRootMarker = ".ferrum-source-depot-root";


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


        bool AppendLogicalInput(festd::vector<std::byte>& result, const festd::span<const std::byte> bytes)
        {
            const uint32_t oldSize = result.size();
            if (oldSize > Constants::kMaxU32 - sizeof(uint64_t) || bytes.size() > Constants::kMaxU32 - oldSize - sizeof(uint64_t))
                return false;

            const uint64_t byteSize = bytes.size();
            result.resize(oldSize + sizeof(byteSize) + static_cast<uint32_t>(bytes.size()));
            memcpy(result.data() + oldSize, &byteSize, sizeof(byteSize));
            memcpy(result.data() + oldSize + sizeof(byteSize), bytes.data(), bytes.size());
            return true;
        }


        bool PathsHaveEqualPrefix(const festd::string_view lhs, const festd::string_view rhs)
        {
            if (lhs.size() < rhs.size())
                return false;

            for (uint32_t index = 0; index < rhs.size(); ++index)
            {
                if (ASCII::ToLower(lhs.byte_at(index)) != ASCII::ToLower(rhs.byte_at(index)))
                    return false;
            }
            return true;
        }


        bool MakeSourceReference(const IO::Path& sourceRoot, const IO::Path& sourcePath, IO::Path& result)
        {
            const festd::string_view root = sourceRoot;
            const festd::string_view source = sourcePath;
            if (root.empty() || source.size() <= root.size() || !PathsHaveEqualPrefix(source, root))
                return false;

            uint32_t relativeOffset = root.size();
            if (!PathParser::IsPathSeparator(root.byte_at(root.size() - 1)))
            {
                if (!PathParser::IsPathSeparator(source.byte_at(relativeOffset)))
                    return false;
                ++relativeOffset;
            }

            result = source.substr(relativeOffset);
            return !result.empty();
        }


        bool FindSourceRoot(const IO::Path& sourcePath, IO::Path& result)
        {
            IO::Path directory = IO::NormalizePath(IO::PathView(sourcePath).parent_directory());
            while (!directory.empty())
            {
                if (IO::File::Exists(directory / kSourceDepotRootMarker))
                {
                    result = directory;
                    return true;
                }

                const IO::Path parent = IO::NormalizePath(IO::PathView(directory).parent_directory());
                if (parent == directory)
                    break;
                directory = parent;
            }

            Logger::LogError("Source '{}' is not inside a depot marked by '{}'", sourcePath, kSourceDepotRootMarker);
            return false;
        }


        bool ResolveSourceReference(const IO::Path& sourceRoot, const IO::Path& reference, IO::Path& result)
        {
            if (reference.empty() || IO::PathView(reference).is_absolute())
            {
                Logger::LogError("Source reference '{}' must be relative to the source depot root", reference);
                return false;
            }

            result = IO::NormalizePath(sourceRoot / reference);
            IO::Path verifiedReference;
            if (MakeSourceReference(sourceRoot, result, verifiedReference))
                return true;

            Logger::LogError("Source reference '{}' escapes depot root '{}'", reference, sourceRoot);
            return false;
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


        const AssetFileArtifact* FindPreviousArtifact(const AssetFile* previous, const festd::string_view productKey,
                                                      const festd::string_view legacyName, const Rtti::TypeID typeId)
        {
            if (previous == nullptr)
                return nullptr;

            for (const AssetFileArtifact& artifact : previous->m_artifacts)
            {
                const bool productKeyMatches = artifact.m_productKey == productKey;
                const bool legacyNameMatches = artifact.m_productKey.empty() && artifact.m_name == legacyName;
                if ((productKeyMatches || legacyNameMatches) && artifact.m_assetTypeId == typeId)
                    return &artifact;
            }
            return nullptr;
        }


        bool InitializeArtifact(AssetFileArtifact& artifact, const AssetFile* previous, const festd::string_view productKey,
                                const festd::string_view name, const Rtti::TypeID typeId)
        {
            artifact.m_productKey = productKey;
            artifact.m_name = name;
            artifact.m_assetTypeId = typeId;
            if (const AssetFileArtifact* oldArtifact = FindPreviousArtifact(previous, productKey, name, typeId))
            {
                artifact.m_assetId = oldArtifact->m_assetId;
                artifact.m_buildSettings = oldArtifact->m_buildSettings;
                artifact.m_lastBuildKey = oldArtifact->m_lastBuildKey;
                artifact.m_builtArtifactId = oldArtifact->m_builtArtifactId;
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


        bool HasArtifact(const AssetFile& assetFile, const festd::string_view productKey, const Rtti::TypeID typeId)
        {
            for (const AssetFileArtifact& artifact : assetFile.m_artifacts)
            {
                if (artifact.m_productKey == productKey && artifact.m_assetTypeId == typeId)
                    return true;
            }
            return false;
        }


        void AppendRemovedArtifacts(AssetFile& assetFile, const AssetFile* previous)
        {
            if (previous == nullptr)
                return;

            for (const AssetFileArtifact& oldArtifact : previous->m_artifacts)
            {
                bool wasRediscovered = false;
                for (const AssetFileArtifact& artifact : assetFile.m_artifacts)
                    wasRediscovered |= artifact.m_assetId == oldArtifact.m_assetId;
                if (wasRediscovered)
                    continue;

                AssetFileArtifact& removedArtifact = assetFile.m_artifacts.emplace_back(oldArtifact);
                removedArtifact.m_dependencies.clear();
                removedArtifact.m_isRemoved = true;
            }
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


        bool ImportTexture(const IO::Path& sourcePath, const IO::Path& sourceReference, const IO::Path& assetFilePath)
        {
            festd::vector<std::byte> sourceBytes;
            if (!ReadFile(sourcePath, sourceBytes) || !ValidateTextureSource(sourcePath, sourceBytes))
                return false;

            AssetFile previous;
            const AssetFile* previousPtr;
            if (!LoadPreviousAssetFile(assetFilePath, previous, previousPtr))
                return false;

            AssetFile assetFile;
            assetFile.m_sourcePath = sourceReference;
            AssetFileArtifact& texture = assetFile.m_artifacts.emplace_back();
            if (!InitializeArtifact(texture, previousPtr, "Texture/Primary", "Texture", Rtti::GetTypeID<TextureAsset>()))
                return false;
            texture.m_buildSettings.Emplace<TextureBuildSettings>();
            AppendRemovedArtifacts(assetFile, previousPtr);

            if (!SaveAssetFile(assetFilePath, assetFile))
            {
                Logger::LogError("Failed to write asset file '{}'", assetFilePath);
                return false;
            }

            Logger::LogInfo("Imported texture '{}' as '{}'", sourcePath, assetFilePath);
            return true;
        }


        bool ImportMaterial(const IO::Path& sourcePath, const IO::Path& sourceReference, const IO::Path& assetFilePath)
        {
            festd::vector<std::byte> sourceBytes;
            if (!ReadFile(sourcePath, sourceBytes) || !ValidateMaterialSource(sourcePath, sourceBytes))
                return false;

            AssetFile previous;
            const AssetFile* previousPtr;
            if (!LoadPreviousAssetFile(assetFilePath, previous, previousPtr))
                return false;

            AssetFile assetFile;
            assetFile.m_sourcePath = sourceReference;
            AssetFileArtifact& material = assetFile.m_artifacts.emplace_back();
            if (!InitializeArtifact(material, previousPtr, "Material/Primary", "Material", Rtti::GetTypeID<MaterialAsset>()))
                return false;
            material.m_buildSettings.Emplace<MaterialBuildSettings>();
            AppendRemovedArtifacts(assetFile, previousPtr);
            return SaveAssetFile(assetFilePath, assetFile);
        }


        void AddExternalDependency(AssetFileArtifact& artifact, const IO::AssetID assetId, const Rtti::TypeID typeId)
        {
            AssetFileDependency& dependency = artifact.m_dependencies.emplace_back();
            dependency.m_assetId = assetId;
            dependency.m_expectedTypeId = typeId;
        }


        bool ImportMaterialInstance(const IO::Path& sourcePath, const IO::Path& sourceReference, const IO::Path& assetFilePath)
        {
            festd::vector<std::byte> sourceBytes;
            if (!ReadFile(sourcePath, sourceBytes))
                return false;
            MaterialInstanceAsset sourceInstance;
            if (!ParseMaterialInstanceSource(sourcePath, sourceBytes, sourceInstance))
                return false;

            AssetFile previous;
            const AssetFile* previousPtr;
            if (!LoadPreviousAssetFile(assetFilePath, previous, previousPtr))
                return false;

            AssetFile assetFile;
            assetFile.m_sourcePath = sourceReference;
            AssetFileArtifact& instance = assetFile.m_artifacts.emplace_back();
            if (!InitializeArtifact(instance,
                                    previousPtr,
                                    "MaterialInstance/Primary",
                                    "MaterialInstance",
                                    Rtti::GetTypeID<MaterialInstanceAsset>()))
                return false;
            instance.m_buildSettings.Emplace<MaterialInstanceBuildSettings>();
            AddExternalDependency(instance, sourceInstance.m_material.GetAssetID(), Rtti::GetTypeID<MaterialAsset>());
            for (const MaterialParameterValue& parameter : sourceInstance.m_parameters)
            {
                if (parameter.m_texture.GetAssetID().IsValid())
                    AddExternalDependency(instance, parameter.m_texture.GetAssetID(), Rtti::GetTypeID<TextureAsset>());
            }
            AppendRemovedArtifacts(assetFile, previousPtr);
            return SaveAssetFile(assetFilePath, assetFile);
        }


        bool ImportModel(const IO::Path& sourcePath, const IO::Path& sourceReference, const IO::Path& sourceRoot,
                         const IO::Path& assetFilePath)
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
            assetFile.m_sourcePath = sourceReference;

            for (const IO::Path& dependencyPath : scene->m_sourcePaths)
            {
                IO::Path dependencyReference;
                if (!MakeSourceReference(sourceRoot, IO::NormalizePath(dependencyPath), dependencyReference))
                {
                    Logger::LogError("Referenced source '{}' is outside depot root '{}'", dependencyPath, sourceRoot);
                    return false;
                }
                assetFile.m_sourceDependencies.push_back(dependencyReference);
            }

            festd::inline_vector<uint32_t, 4> textureArtifactIndices;
            for (uint32_t textureIndex = 0; textureIndex < scene->m_texturePaths.size(); ++textureIndex)
            {
                festd::vector<std::byte> textureBytes;
                const IO::Path& texturePath = scene->m_texturePaths[textureIndex];
                if (!ReadFile(texturePath, textureBytes) || !ValidateTextureSource(texturePath, textureBytes))
                    return false;

                IO::Path textureReference;
                if (!MakeSourceReference(sourceRoot, IO::NormalizePath(texturePath), textureReference))
                {
                    Logger::LogError("Referenced source '{}' is outside depot root '{}'", texturePath, sourceRoot);
                    return false;
                }

                const auto productKey = Fmt::FixedFormat("Texture/{}", textureReference);
                AssetFileArtifact& texture = assetFile.m_artifacts.emplace_back();
                if (!InitializeArtifact(texture, previousPtr, productKey, productKey, Rtti::GetTypeID<TextureAsset>()))
                    return false;
                TextureBuildSettings textureSettings;
                textureSettings.m_sourceObjectIndex = textureIndex;
                textureSettings.m_sourcePath = textureReference;
                texture.m_buildSettings.Emplace<TextureBuildSettings>(std::move(textureSettings));
                textureArtifactIndices.push_back(assetFile.m_artifacts.size() - 1);
            }

            for (uint32_t modelIndex = 0; modelIndex < scene->m_models.size(); ++modelIndex)
            {
                const IntermediateModel& importedModel = scene->m_models[modelIndex];
                if (importedModel.m_productKey.empty())
                {
                    Logger::LogError("Model '{}' contains an unnamed product with no stable import key", sourcePath);
                    return false;
                }

                const auto meshKey = Fmt::FixedFormat("Mesh/{}", importedModel.m_productKey);
                if (HasArtifact(assetFile, meshKey, Rtti::GetTypeID<MeshAsset>()))
                {
                    Logger::LogError("Model '{}' contains duplicate import product key '{}'", sourcePath, meshKey);
                    return false;
                }
                AssetFileArtifact& mesh = assetFile.m_artifacts.emplace_back();
                if (!InitializeArtifact(mesh, previousPtr, meshKey, meshKey, Rtti::GetTypeID<MeshAsset>()))
                    return false;
                MeshBuildSettings meshSettings;
                if (const auto* previousSettings = mesh.m_buildSettings.TryGet<MeshBuildSettings>())
                    meshSettings = *previousSettings;
                meshSettings.m_sourceObjectIndex = modelIndex;
                mesh.m_buildSettings.Emplace<MeshBuildSettings>(meshSettings);
                const uint32_t meshArtifactIndex = assetFile.m_artifacts.size() - 1;

                const auto modelKey = Fmt::FixedFormat("Model/{}", importedModel.m_productKey);
                if (HasArtifact(assetFile, modelKey, Rtti::GetTypeID<ModelAsset>()))
                {
                    Logger::LogError("Model '{}' contains duplicate import product key '{}'", sourcePath, modelKey);
                    return false;
                }
                AssetFileArtifact& model = assetFile.m_artifacts.emplace_back();
                if (!InitializeArtifact(model, previousPtr, modelKey, modelKey, Rtti::GetTypeID<ModelAsset>()))
                    return false;
                ModelBuildSettings modelSettings;
                modelSettings.m_sourceObjectIndex = modelIndex;
                model.m_buildSettings.Emplace<ModelBuildSettings>(modelSettings);
                AddDependency(model, assetFile.m_artifacts[meshArtifactIndex]);
                for (const uint32_t textureArtifactIndex : textureArtifactIndices)
                    AddDependency(model, assetFile.m_artifacts[textureArtifactIndex]);
            }

            AppendRemovedArtifacts(assetFile, previousPtr);

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


        bool SerializeBuildSettings(const AssetFileArtifact& artifact, festd::vector<std::byte>& result)
        {
            MemoryOutputStream stream;
            Serialization::TaggedBinaryFormat format;
            Serialization::SerializationContext context(&stream, format);
            if (context.Store(artifact.m_buildSettings) != Serialization::ResultCode::kSuccess)
                return false;

            const festd::span<const std::byte> data = stream.GetData();
            result.assign(data.begin(), data.end());
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
            uint64_t materialSchemaHash = 0;
            if (artifact.m_assetTypeId == Rtti::GetTypeID<MaterialAsset>())
                materialSchemaHash = Serialization::GetSchemaHash<MaterialAsset>();
            else if (artifact.m_assetTypeId == Rtti::GetTypeID<MaterialInstanceAsset>())
                materialSchemaHash = Serialization::GetSchemaHash<MaterialInstanceAsset>();
            if (materialSchemaHash != 0)
            {
                lowHasher.Update(materialSchemaHash);
                highHasher.Update(materialSchemaHash);
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


        struct BuildRequest final
        {
            const AssetFileArtifact& m_artifact;
            IO::Path m_sourcePath;
            festd::span<const std::byte> m_sourceData;
            IO::Path m_outputDirectory;
            IO::ArtifactID* m_resultArtifactId;
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
            settings.m_resultArtifactId = request.m_resultArtifactId;
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
            settings.m_resultArtifactId = request.m_resultArtifactId;
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
            settings.m_resultArtifactId = request.m_resultArtifactId;
            settings.m_sourceData = request.m_sourceData;
            return ProcessTexture(settings);
        }


        bool BuildMaterialArtifact(const BuildRequest& request)
        {
            if (request.m_artifact.m_buildSettings.TryGet<MaterialBuildSettings>() == nullptr)
                return false;

            MaterialProcessSettings settings;
            settings.m_inputFile = request.m_sourcePath;
            settings.m_outputDirectory = request.m_outputDirectory;
            settings.m_assetId = request.m_artifact.m_assetId;
            settings.m_resultArtifactId = request.m_resultArtifactId;
            settings.m_sourceData = request.m_sourceData;
            return ProcessMaterial(settings);
        }


        bool BuildMaterialInstanceArtifact(const BuildRequest& request)
        {
            if (request.m_artifact.m_buildSettings.TryGet<MaterialInstanceBuildSettings>() == nullptr)
                return false;

            MaterialProcessSettings settings;
            settings.m_inputFile = request.m_sourcePath;
            settings.m_outputDirectory = request.m_outputDirectory;
            settings.m_assetId = request.m_artifact.m_assetId;
            settings.m_resultArtifactId = request.m_resultArtifactId;
            settings.m_sourceData = request.m_sourceData;
            return ProcessMaterialInstance(settings);
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
                { Rtti::GetTypeID<MaterialAsset>(), BuildMaterialArtifact },
                { Rtti::GetTypeID<MaterialInstanceAsset>(), BuildMaterialInstanceArtifact },
            };
            for (const BuilderEntry& builder : builders)
            {
                if (builder.m_typeId == typeId)
                    return builder.m_function;
            }
            return nullptr;
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

                IO::Path sourcePath = m_primarySourcePath;
                festd::span<const std::byte> sourceData = m_primarySourceData;
                festd::span<const std::byte> logicalInputData = m_primaryLogicalInputData;
                festd::vector<std::byte> externalSourceData;
                festd::vector<std::byte> materialLogicalInput;
                if (const auto* textureSettings = artifact.m_buildSettings.TryGet<TextureBuildSettings>();
                    textureSettings != nullptr && !textureSettings->m_sourcePath.empty())
                {
                    if (!ResolveSourceReference(m_sourceRoot, textureSettings->m_sourcePath, sourcePath))
                        return false;
                    if (!ReadFile(sourcePath, externalSourceData))
                    {
                        Logger::LogError("Failed to read source '{}' for product '{}'", sourcePath, artifact.m_name);
                        return false;
                    }
                    sourceData = externalSourceData;
                    logicalInputData = externalSourceData;
                }

                if (artifact.m_assetTypeId == Rtti::GetTypeID<MaterialAsset>())
                {
                    if (!AppendLogicalInput(materialLogicalInput, logicalInputData))
                        return false;
                    constexpr const char* modules[] = { "material", "specializer", "technique", "pipeline", "set" };
                    for (const char* module : modules)
                    {
                        IO::Path path(FE_MATERIAL_LIBRARY_DIR);
                        path /= Fmt::FixedFormat("{}.luau", module);
                        festd::vector<std::byte> bytes;
                        if (!ReadFile(path, bytes) || !AppendLogicalInput(materialLogicalInput, bytes))
                            return false;
                    }
                    logicalInputData = materialLogicalInput;
                }

                festd::vector<std::byte> settingsData;
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
                const IO::BuildKey buildKey = ComputeBuildKey(artifact, logicalInputData, settingsData, dependencyArtifactIds);

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
                const BuildRequest request{ artifact, sourcePath, sourceData, m_stagingOutputDirectory, &artifactId };
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


    bool ImportAsset(const ImportAssetSettings& settings)
    {
        const IO::Path sourcePath = IO::GetAbsolutePath(settings.m_inputFile);
        const IO::Path assetFilePath = IO::GetAbsolutePath(settings.m_outputFile);
        if (IO::PathView(assetFilePath).extension() != ".asset")
        {
            Logger::LogError("Import output '{}' is not an .asset file", assetFilePath);
            return false;
        }

        IO::Path sourceRoot;
        if (!FindSourceRoot(sourcePath, sourceRoot))
            return false;

        IO::Path sourceReference;
        if (!MakeSourceReference(sourceRoot, sourcePath, sourceReference))
        {
            Logger::LogError("Source '{}' is outside depot root '{}'", sourcePath, sourceRoot);
            return false;
        }

        const IO::PathView sourceView(sourcePath);
        if (sourceView.extension() == ".gltf" || sourceView.extension() == ".glb")
            return ImportModel(sourcePath, sourceReference, sourceRoot, assetFilePath);
        if (sourceView.extension() == ".dds")
            return ImportTexture(sourcePath, sourceReference, assetFilePath);
        if (sourceView.extension() == ".luau" && sourceView.stem().ends_with(".mat"))
            return ImportMaterial(sourcePath, sourceReference, assetFilePath);
        if (sourceView.extension() == ".luau" && sourceView.stem().ends_with(".matinst"))
            return ImportMaterialInstance(sourcePath, sourceReference, assetFilePath);

        Logger::LogError("Unsupported source '{}'. Expected .gltf, .glb, .dds, .mat.luau, or .matinst.luau", sourcePath);
        return false;
    }


    bool BuildAsset(const BuildAssetSettings& settings)
    {
        if (settings.m_sourceRoot.empty())
        {
            Logger::LogError("A source depot root is required to build an asset");
            return false;
        }
        if (settings.m_outputDirectory.empty())
        {
            Logger::LogError("An artifact output directory is required to build an asset");
            return false;
        }

        const IO::Path assetFilePath = IO::GetAbsolutePath(settings.m_assetFile);
        const IO::Path sourceRoot = IO::GetAbsolutePath(settings.m_sourceRoot);
        const IO::Path outputDirectory = IO::GetAbsolutePath(settings.m_outputDirectory);
        if (IO::PathView(assetFilePath).extension() != ".asset")
        {
            Logger::LogError("Build input '{}' is not an .asset file", assetFilePath);
            return false;
        }
        if (!IO::File::Exists(sourceRoot / kSourceDepotRootMarker))
        {
            Logger::LogError("Source root '{}' does not contain '{}'", sourceRoot, kSourceDepotRootMarker);
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
        festd::vector<std::byte> logicalInputBytes;
        IO::Path sourcePath;
        if (assetFile.m_sourcePath.empty())
        {
            sourcePath = assetFilePath;
            sourceBytes = assetFile.m_embeddedSourceData;
            if (!AppendLogicalInput(logicalInputBytes, sourceBytes))
                return false;
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
