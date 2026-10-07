#include <AssetBuilder/PipelineInternal.h>
#include <Core/IO/MemoryStream.h>
#include <Core/Logging/Logger.h>
#include <Core/Serialization/BinarySerialization.h>
#include <Framework/Entities/EntityCollection.h>
#include <GameFramework/TransformComponents.h>

namespace FE::AssetBuilder::Internal
{
    static void CollectSerializedAssetDependency(void* userData, const Uuid assetId, const Rtti::TypeID expectedTypeId,
                                                 const uint32_t dependencyKind)
    {
        auto* artifact = static_cast<AssetFileArtifact*>(userData);
        const auto kind = static_cast<IO::DependencyKind>(dependencyKind);
        for (const AssetFileDependency& dependency : artifact->m_dependencies)
        {
            if (dependency.m_assetId == assetId && dependency.m_expectedTypeId == expectedTypeId && dependency.m_kind == kind)
            {
                return;
            }
        }

        AssetFileDependency& dependency = artifact->m_dependencies.emplace_back();
        dependency.m_assetId = assetId;
        dependency.m_expectedTypeId = expectedTypeId;
        dependency.m_kind = kind;
    }


    bool RefreshSerializedAssetDependencies(AssetFileArtifact& artifact)
    {
        const Rtti::Type* type = artifact.m_buildSettings.GetType();
        if (type == nullptr || type->m_id != artifact.m_assetTypeId)
            return false;

        artifact.m_dependencies.clear();
        IO::WriteOnlyMemoryStream stream;
        Serialization::TaggedBinaryFormat format;
        Serialization::SerializationContext context(&stream, format, &artifact, CollectSerializedAssetDependency);
        if (context.Store(*type, artifact.m_buildSettings.GetValue()) == Serialization::ResultCode::kSuccess)
            return true;

        Logger::LogError("Failed to inspect dependencies for serialized asset type {}", artifact.m_assetTypeId);
        return false;
    }


    bool ImportSerializedAsset(const Rtti::TypeID assetTypeId, const IO::Path& assetFilePath)
    {
        (void)Rtti::GetType<Framework::EntityCollection>();
        (void)Rtti::GetType<Framework::EntityCollectionInstanceAsset>();
        (void)Rtti::GetType<GameFramework::TransformComponent>();
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
