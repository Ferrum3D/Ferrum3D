#include <Core/IO/Artifact.h>
#include <Core/IO/FileStream.h>

namespace FE::IO
{
    struct ArtifactStore::Impl final
    {
        Path m_assetDirectoryPath;
    };

    ArtifactStore::Impl* ArtifactStore::GImpl = nullptr;


    void ArtifactStore::Init()
    {
        FE_Assert(GImpl == nullptr, "Artifact Store already initialized");
        GImpl = Memory::DefaultNew<Impl>();
    }


    void ArtifactStore::Shutdown()
    {
        FE_Assert(GImpl != nullptr, "Artifact Store not initialized");
        Memory::DefaultDelete(GImpl);
        GImpl = nullptr;
    }


    ResolvedDataSource ArtifactStore::ResolveMeta(const AssetID assetID)
    {
        const auto idString = Str::ToLower<festd::fixed_string>(Fmt::FixedFormat("{}", assetID));

        ResolvedDataSource artifactMetadataSource;
        Fmt::FormatTo(artifactMetadataSource.m_filePath,
                      "artifacts/{}/{}/{}.meta",
                      idString.substr_ascii(0, 2),
                      idString.substr_ascii(2, 2),
                      idString);

        artifactMetadataSource.m_filePath = GImpl->m_assetDirectoryPath / artifactMetadataSource.m_filePath;
        return artifactMetadataSource;
    }


    void ArtifactStore::SetCatalogSource(const festd::string_view assetDirectoryPath)
    {
        GImpl->m_assetDirectoryPath = assetDirectoryPath;
    }
} // namespace FE::IO
