#pragma once
#include <Core/IO/Artifact.h>
#include <Core/IO/Path.h>
#include <festd/span.h>

namespace FE::Graphics
{
    struct MaterialInstanceAsset;
}

namespace FE::AssetBuilder
{
    struct MaterialBuildSettings final
    {
        FE_RTTI("A5F968E2-282C-4364-957B-DC52CC51014D");
        FE_RTTI_Reflect();
        FE_RTTI_Serialize();
    };


    struct MaterialInstanceBuildSettings final
    {
        FE_RTTI("42707CCB-1248-4ABB-A6A8-067B1D2D6A0B");
        FE_RTTI_Reflect();
        FE_RTTI_Serialize();
    };


    struct MaterialProcessSettings final
    {
        IO::Path m_inputFile;
        IO::Path m_outputDirectory;
        IO::AssetID m_assetId = IO::AssetID::kNull;
        IO::ArtifactID* m_resultArtifactId = nullptr;
        festd::span<const std::byte> m_sourceData;
    };


    bool ValidateMaterialSource(const IO::Path& path, festd::span<const std::byte> sourceData);
    bool ProcessMaterial(const MaterialProcessSettings& settings);
    bool ParseMaterialInstanceSource(const IO::Path& path, festd::span<const std::byte> sourceData,
                                     Graphics::MaterialInstanceAsset& result);
    bool ProcessMaterialInstance(const MaterialProcessSettings& settings);
} // namespace FE::AssetBuilder
