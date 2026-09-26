#include <AssetBuilder/MaterialProcessor.h>
#include <AssetBuilder/PipelineInternal.h>
#include <Graphics/Assets/MaterialAssets.h>

using namespace FE::Graphics;

namespace FE::AssetBuilder::Internal
{
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
} // namespace FE::AssetBuilder::Internal
