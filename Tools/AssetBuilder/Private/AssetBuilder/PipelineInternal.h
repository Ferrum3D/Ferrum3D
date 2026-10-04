#pragma once
#include <AssetBuilder/AssetFile.h>
#include <Core/IO/Path.h>
#include <festd/span.h>
#include <festd/vector.h>

namespace FE::AssetBuilder::Internal
{
    inline constexpr festd::string_view kSourceDepotRootMarker = ".ferrum-source-depot-root";

    bool ReadFile(const IO::Path& path, festd::vector<std::byte>& bytes);
    bool AppendLogicalInput(festd::vector<std::byte>& result, festd::span<const std::byte> bytes);
    bool PathsHaveEqualPrefix(festd::string_view lhs, festd::string_view rhs);
    bool MakeSourceReference(const IO::Path& sourceRoot, const IO::Path& sourcePath, IO::Path& result);
    bool FindSourceRoot(const IO::Path& sourcePath, IO::Path& result);
    bool ResolveSourceReference(const IO::Path& sourceRoot, const IO::Path& reference, IO::Path& result);
    IO::AssetID GenerateAssetId();
    bool InitializeArtifact(AssetFileArtifact& artifact, const AssetFile* previous, festd::string_view productKey,
                            festd::string_view name, Rtti::TypeID typeId);
    bool HasArtifact(const AssetFile& assetFile, festd::string_view productKey, Rtti::TypeID typeId);
    void AppendRemovedArtifacts(AssetFile& assetFile, const AssetFile* previous);
    void AddDependency(AssetFileArtifact& artifact, const AssetFileArtifact& dependency);
    bool LoadPreviousAssetFile(const IO::Path& path, AssetFile& previous, const AssetFile*& previousPtr);

    bool ImportTexture(const IO::Path& sourcePath, const IO::Path& sourceReference, const IO::Path& assetFilePath);
    bool ImportMaterial(const IO::Path& sourcePath, const IO::Path& sourceReference, const IO::Path& assetFilePath);
    bool ImportMaterialInstance(const IO::Path& sourcePath, const IO::Path& sourceReference, const IO::Path& assetFilePath);
    bool ImportModel(const IO::Path& sourcePath, const IO::Path& sourceReference, const IO::Path& sourceRoot,
                     const IO::Path& assetFilePath);
    bool ImportSerializedAsset(Rtti::TypeID assetTypeId, const IO::Path& assetFilePath);
    bool RefreshSerializedAssetDependencies(AssetFileArtifact& artifact);

    struct BuildRequest final
    {
        const AssetFileArtifact& m_artifact;
        IO::Path m_sourcePath;
        festd::span<const std::byte> m_sourceData;
        IO::Path m_outputDirectory;
        IO::ArtifactID* m_resultArtifactId;
    };


    using BuildFunction = bool (*)(const BuildRequest& request);

    struct AssetCompiler final
    {
        BuildFunction m_build = nullptr;
        uint32_t m_version = 0;
    };


    AssetCompiler FindCompiler(Rtti::TypeID typeId);


    struct BuildInputs final
    {
        IO::Path m_sourcePath;
        festd::span<const std::byte> m_sourceData;
        festd::span<const std::byte> m_logicalInputData;
        festd::vector<std::byte> m_externalSourceData;
        festd::vector<std::byte> m_materialLogicalInput;
    };

    bool PrepareBuildInputs(const AssetFileArtifact& artifact, const IO::Path& sourceRoot, const IO::Path& primarySourcePath,
                            festd::span<const std::byte> primarySourceData, festd::span<const std::byte> primaryLogicalInputData,
                            BuildInputs& result);
} // namespace FE::AssetBuilder::Internal
