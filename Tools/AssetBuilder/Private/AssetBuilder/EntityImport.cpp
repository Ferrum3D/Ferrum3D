#include <AssetBuilder/EntityImport.h>
#include <AssetBuilder/PipelineInternal.h>

namespace FE::AssetBuilder
{
    namespace
    {
        template<class T>
        bool ImportDefinition(const IO::Path& output, const T& definition)
        {
            const IO::Path path = IO::GetAbsolutePath(output);
            if (IO::PathView(path).extension() != ".asset")
                return false;
            AssetFile previous;
            const AssetFile* previousPtr;
            if (!Internal::LoadPreviousAssetFile(path, previous, previousPtr))
                return false;
            AssetFile assetFile;
            auto& artifact = assetFile.m_artifacts.emplace_back();
            const auto& type = Rtti::GetType<T>();
            if (!Internal::InitializeArtifact(artifact, previousPtr, "Serialized/Primary", type.m_name, type.m_id))
                return false;
            artifact.m_buildSettings.Emplace<T>(definition);
            if (!Internal::RefreshSerializedAssetDependencies(artifact))
                return false;
            Internal::AppendRemovedArtifacts(assetFile, previousPtr);
            return SaveAssetFile(path, assetFile);
        }
    } // namespace


    bool ImportEntityCollection(const IO::Path& output, const Framework::EntityCollection& collection)
    {
        return collection.ValidatePayloads() && ImportDefinition(output, collection);
    }


    bool ImportEntityPlacement(const IO::Path& output, const Framework::EntityCollectionInstanceAsset& placement,
                               const Framework::EntityCollection& collection)
    {
        return placement.Validate(collection) && placement.m_root.ValidatePayloads() && collection.ValidatePayloads()
            && ImportDefinition(output, placement);
    }
} // namespace FE::AssetBuilder
