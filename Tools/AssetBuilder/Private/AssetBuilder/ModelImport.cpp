#include <AssetBuilder/ModelImporter.h>
#include <AssetBuilder/ModelProcessor.h>
#include <AssetBuilder/PipelineInternal.h>
#include <AssetBuilder/TextureProcessor.h>
#include <Graphics/Assets/Assets.h>

using namespace FE::Graphics;

namespace FE::AssetBuilder::Internal
{
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
        if (scene == nullptr)
            return false;

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
} // namespace FE::AssetBuilder::Internal
