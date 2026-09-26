#include <AssetBuilder/PipelineInternal.h>
#include <AssetBuilder/TextureProcessor.h>
#include <Graphics/Assets/Assets.h>

using namespace FE::Graphics;

namespace FE::AssetBuilder::Internal
{
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
} // namespace FE::AssetBuilder::Internal
