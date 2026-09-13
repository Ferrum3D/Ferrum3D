#pragma once
#include <Core/IO/Artifact.h>
#include <Core/IO/Path.h>
#include <Core/RTTI/Any.h>
#include <festd/string.h>
#include <festd/vector.h>

namespace FE::AssetBuilder
{
    struct AssetFileDependency final
    {
        IO::AssetID m_assetId = IO::AssetID::kNull;
        Rtti::TypeID m_expectedTypeId = Rtti::TypeID::kNull;
        IO::DependencyKind m_kind = IO::DependencyKind::kHard;

        FE_RTTI_Reflect("7C6543F2-90A8-45F5-B93D-C760606B3132");
        FE_RTTI_Serialize();
    };


    //! One artifact-producing product discovered in a source asset.
    struct AssetFileArtifact final
    {
        festd::string m_name;
        IO::AssetID m_assetId = IO::AssetID::kNull;
        Rtti::TypeID m_assetTypeId = Rtti::TypeID::kNull;
        Rtti::Any m_buildSettings;
        festd::inline_vector<AssetFileDependency, 4> m_dependencies;

        FE_RTTI_Reflect("5EC89B68-372E-4388-BD0D-869C03C9C65D");
        FE_RTTI_Serialize();
    };


    //! Authoring metadata for every artifact produced from one source.
    //!
    //! Its filesystem location never participates in asset or artifact identity. An empty source path denotes an engine-authored
    //! asset; only those descriptors may contain embedded source data.
    struct AssetFile final
    {
        IO::Path m_sourcePath;
        festd::inline_vector<AssetFileArtifact, 4> m_artifacts;
        festd::inline_vector<IO::ArtifactID, 4> m_builtArtifactIds;
        festd::vector<std::byte> m_embeddedSourceData;

        FE_RTTI_Reflect("196D6B77-484F-429B-A74F-0E99C4F367B2");
        FE_RTTI_Serialize();
    };


    bool LoadAssetFile(const IO::Path& path, AssetFile& result);
    bool SaveAssetFile(const IO::Path& path, const AssetFile& assetFile);
} // namespace FE::AssetBuilder
