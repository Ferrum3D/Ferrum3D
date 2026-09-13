#pragma once
#include <AssetBuilder/AssetFile.h>
#include <Core/IO/BaseIO.h>
#include <Core/IO/Path.h>
#include <Core/Logging/Logger.h>
#include <festd/span.h>

namespace FE::AssetBuilder
{
    struct ModelBuildSettings final
    {
        uint32_t m_sourceObjectIndex = kInvalidIndex;

        FE_RTTI_Reflect("80D1A492-1C42-4C3C-9913-DF8085E9655A");
        FE_RTTI_Serialize();
    };


    struct MeshBuildSettings final
    {
        uint32_t m_sourceObjectIndex = kInvalidIndex;
        bool m_generateLods = true;

        FE_RTTI_Reflect("534CD65C-1645-484D-B124-37E908676F00");
        FE_RTTI_Serialize();
    };


    struct MeshProcessSettings final
    {
        IO::Path m_inputFile;
        IO::Path m_outputDirectory;
        IO::AssetID m_assetId = IO::AssetID::kNull;
        IO::ArtifactID m_artifactId = IO::ArtifactID::kNull;
        uint32_t m_sourceObjectIndex = kInvalidIndex;
        bool m_generateLods = true;
        festd::span<const std::byte> m_sourceData;
    };

    struct ModelProcessSettings final
    {
        IO::Path m_outputDirectory;
        IO::AssetID m_assetId = IO::AssetID::kNull;
        IO::ArtifactID m_artifactId = IO::ArtifactID::kNull;
        festd::span<const AssetFileDependency> m_dependencies;
    };


    bool ProcessModel(const ModelProcessSettings& settings);
    bool ProcessMesh(const MeshProcessSettings& settings);
} // namespace FE::AssetBuilder
