#include <AssetBuilder/ArtifactWriter.h>
#include <AssetBuilder/MaterialProcessor.h>
#include <AssetBuilder/ModelProcessor.h>
#include <AssetBuilder/PipelineInternal.h>
#include <AssetBuilder/TextureProcessor.h>
#include <Graphics/Assets/Assets.h>
#include <Graphics/Assets/MaterialAssets.h>

using namespace FE::Graphics;

namespace FE::AssetBuilder::Internal
{
    namespace
    {
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


        bool BuildSerializedArtifact(const BuildRequest& request)
        {
            const Rtti::Type* type = request.m_artifact.m_buildSettings.GetType();
            if (type == nullptr || type->m_id != request.m_artifact.m_assetTypeId)
            {
                Logger::LogError("Product '{}' does not contain serialized build settings for type {}",
                                 request.m_artifact.m_name,
                                 request.m_artifact.m_assetTypeId);
                return false;
            }

            ArtifactWriter writer(request.m_outputDirectory, request.m_artifact.m_assetId, request.m_artifact.m_assetTypeId);
            for (const AssetFileDependency& dependency : request.m_artifact.m_dependencies)
                writer.AddDependency(dependency.m_assetId, dependency.m_expectedTypeId, dependency.m_kind);
            if (!writer.WriteHeader(*type, request.m_artifact.m_buildSettings.GetValue()) || !writer.Finish())
                return false;
            *request.m_resultArtifactId = writer.GetArtifactID();
            return true;
        }


        bool BuildMaterialInstanceArtifact(const BuildRequest& request)
        {
            if (request.m_artifact.m_buildSettings.GetType() != nullptr
                && request.m_artifact.m_buildSettings.GetType()->m_id == request.m_artifact.m_assetTypeId)
            {
                return BuildSerializedArtifact(request);
            }
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


        struct BuilderEntry final
        {
            Rtti::TypeID m_typeId;
            AssetCompiler m_compiler;
        };
    } // namespace


    AssetCompiler FindCompiler(const Rtti::TypeID typeId)
    {
        // Bump an asset type's compiler version when its generated data changes independently of its serialization schema.
        const BuilderEntry builders[] = {
            { Rtti::GetTypeID<ModelAsset>(), { BuildModelArtifact, 1 } },
            { Rtti::GetTypeID<MeshAsset>(), { BuildMeshArtifact, 1 } },
            { Rtti::GetTypeID<TextureAsset>(), { BuildTextureArtifact, 1 } },
            { Rtti::GetTypeID<MaterialAsset>(), { BuildMaterialArtifact, 1 } },
            { Rtti::GetTypeID<MaterialInstanceAsset>(), { BuildMaterialInstanceArtifact, 1 } },
        };
        for (const BuilderEntry& builder : builders)
        {
            if (builder.m_typeId == typeId)
                return builder.m_compiler;
        }
        const Rtti::Type* type = Rtti::TypeRegistry::FindType(typeId);
        if (type != nullptr && type->m_serialize != nullptr)
            return { BuildSerializedArtifact, 1 };
        return {};
    }


    bool PrepareBuildInputs(const AssetFileArtifact& artifact, const IO::Path& sourceRoot, const IO::Path& primarySourcePath,
                            const festd::span<const std::byte> primarySourceData,
                            const festd::span<const std::byte> primaryLogicalInputData, BuildInputs& result)
    {
        result.m_sourcePath = primarySourcePath;
        result.m_sourceData = primarySourceData;
        result.m_logicalInputData = primaryLogicalInputData;

        if (const auto* textureSettings = artifact.m_buildSettings.TryGet<TextureBuildSettings>();
            textureSettings != nullptr && !textureSettings->m_sourcePath.empty())
        {
            if (!ResolveSourceReference(sourceRoot, textureSettings->m_sourcePath, result.m_sourcePath))
                return false;
            if (!ReadFile(result.m_sourcePath, result.m_externalSourceData))
            {
                Logger::LogError("Failed to read source '{}' for product '{}'", result.m_sourcePath, artifact.m_name);
                return false;
            }
            result.m_sourceData = result.m_externalSourceData;
            result.m_logicalInputData = result.m_externalSourceData;
        }

        if (artifact.m_assetTypeId == Rtti::GetTypeID<MaterialAsset>())
        {
            if (!AppendLogicalInput(result.m_materialLogicalInput, result.m_logicalInputData))
                return false;

            constexpr const char* modules[] = { "material", "specializer", "technique", "pipeline", "set" };
            for (const char* module : modules)
            {
                IO::Path path(FE_MATERIAL_LIBRARY_DIR);
                path /= Fmt::FixedFormat("{}.luau", module);

                festd::vector<std::byte> bytes;
                if (!ReadFile(path, bytes) || !AppendLogicalInput(result.m_materialLogicalInput, bytes))
                    return false;
            }

            result.m_logicalInputData = result.m_materialLogicalInput;
        }

        return true;
    }
} // namespace FE::AssetBuilder::Internal
