#include <AssetBuilder/PipelineInternal.h>
#include <Core/IO/AssetReferences.h>
#include <Core/Logging/Logger.h>

namespace FE::AssetBuilder::Internal
{
    static void CollectSerializedAssetDependency(AssetFileArtifact& artifact, const IO::AssetReferenceInfo& reference)
    {
        for (const AssetFileDependency& dependency : artifact.m_dependencies)
        {
            const bool isSameDependency = dependency.m_assetId == reference.m_assetId
                && dependency.m_expectedTypeId == reference.m_assetType && dependency.m_kind == reference.m_kind;
            if (isSameDependency)
                return;
        }

        AssetFileDependency& dependency = artifact.m_dependencies.emplace_back();
        dependency.m_assetId = reference.m_assetId;
        dependency.m_expectedTypeId = reference.m_assetType;
        dependency.m_kind = reference.m_kind;
    }


    bool RefreshSerializedAssetDependencies(AssetFileArtifact& artifact)
    {
        const Rtti::Type* type = artifact.m_buildSettings.GetType();
        if (type == nullptr || type->m_id != artifact.m_assetTypeId)
            return false;

        artifact.m_dependencies.clear();
        IO::VisitAssetReferences(*type, artifact.m_buildSettings.GetValue(), [&](const IO::AssetReferenceInfo& reference) {
            CollectSerializedAssetDependency(artifact, reference);
        });

        return true;
    }


    bool ImportSerializedAsset(const Rtti::TypeID assetTypeId, const IO::Path& assetFilePath)
    {
        const Rtti::Type* type = Rtti::TypeRegistry::FindType(assetTypeId);
        if (type == nullptr)
        {
            Logger::LogError("Asset type {} is not registered", assetTypeId);
            return false;
        }

        if (type->m_defaultConstructor == nullptr || type->m_copyConstructor == nullptr || type->m_destructor == nullptr
            || type->m_serialize == nullptr || type->m_deserialize == nullptr)
        {
            Logger::LogError("Type {} cannot be used as a serialized asset", assetTypeId);
            return false;
        }

        AssetFile previous;
        const AssetFile* previousPtr;
        if (!LoadPreviousAssetFile(assetFilePath, previous, previousPtr))
            return false;

        AssetFile assetFile;
        AssetFileArtifact& artifact = assetFile.m_artifacts.emplace_back();
        if (!InitializeArtifact(artifact, previousPtr, "Serialized/Primary", type->m_name, assetTypeId))
            return false;

        if (artifact.m_buildSettings.GetType() != nullptr && artifact.m_buildSettings.GetType()->m_id != assetTypeId)
        {
            artifact.m_buildSettings.Reset();
        }

        if (!artifact.m_buildSettings.HasValue() && !artifact.m_buildSettings.Emplace(*type))
        {
            Logger::LogError("Failed to default-construct serialized asset type {}", assetTypeId);
            return false;
        }

        if (!RefreshSerializedAssetDependencies(artifact))
            return false;

        AppendRemovedArtifacts(assetFile, previousPtr);
        if (!SaveAssetFile(assetFilePath, assetFile))
        {
            Logger::LogError("Failed to write asset file '{}'", assetFilePath);
            return false;
        }

        Logger::LogInfo("Created serialized asset '{}' for type {}", assetFilePath, assetTypeId);
        return true;
    }
} // namespace FE::AssetBuilder::Internal
