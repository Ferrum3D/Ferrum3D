#pragma once
#include <Core/IO/BaseIO.h>
#include <Core/IO/Path.h>
#include <festd/span.h>

namespace FE::AssetBuilder
{
    struct TextureBuildSettings final
    {
        uint32_t m_sourceObjectIndex = kInvalidIndex;
        IO::Path m_sourcePath;

        FE_RTTI_Reflect("38A7F222-C636-4947-B700-9E6FA582BC4A");
        FE_RTTI_Serialize();
    };


    struct TextureProcessSettings final
    {
        IO::Path m_inputFile;
        IO::Path m_outputDirectory;
        IO::AssetID m_assetId = IO::AssetID::kNull;
        IO::ArtifactID m_artifactId = IO::ArtifactID::kNull;
        festd::span<const std::byte> m_sourceData;
    };


    bool ValidateTextureSource(const IO::Path& path, festd::span<const std::byte> sourceData);
    bool ProcessTexture(const TextureProcessSettings& settings);
} // namespace FE::AssetBuilder
