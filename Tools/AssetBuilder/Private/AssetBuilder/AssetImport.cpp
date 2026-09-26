#include <AssetBuilder/AssetPipeline.h>
#include <AssetBuilder/PipelineInternal.h>
#include <Core/Logging/Logger.h>

namespace FE::AssetBuilder
{
    using namespace Internal;

    bool ImportAsset(const ImportAssetSettings& settings)
    {
        const IO::Path assetFilePath = IO::GetAbsolutePath(settings.m_outputFile);
        if (IO::PathView(assetFilePath).extension() != ".asset")
        {
            Logger::LogError("Import output '{}' is not an .asset file", assetFilePath);
            return false;
        }

        if (settings.m_assetTypeId.IsValid())
        {
            if (!settings.m_inputFile.empty())
            {
                Logger::LogError("A source-less serialized asset cannot also specify a source file");
                return false;
            }
            return ImportSerializedAsset(settings.m_assetTypeId, assetFilePath);
        }

        if (settings.m_inputFile.empty())
        {
            Logger::LogError("An input source or serialized asset type is required");
            return false;
        }

        const IO::Path sourcePath = IO::GetAbsolutePath(settings.m_inputFile);

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
} // namespace FE::AssetBuilder
