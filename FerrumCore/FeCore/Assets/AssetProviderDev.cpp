#include <FeCore/Assets/AssetProviderDev.h>
#include <FeCore/IO/FileStream.h>

namespace FE::Assets
{
    void AssetProviderDev::AttachRegistry(const Shared<AssetRegistry>& registry)
    {
        m_Registry = registry;
    }

    void AssetProviderDev::DetachRegistry()
    {
        m_Registry.Reset();
    }

    Shared<IO::IStream> AssetProviderDev::CreateAssetLoadingStream(const AssetID& assetID)
    {
        FE_ASSERT_MSG(m_Registry, "Registry was not attached");
        auto stream = MakeShared<IO::FileStream>(MakeShared<IO::FileHandle>());
        stream->Open(m_Registry->GetAssetFilePath(assetID), IO::OpenMode::ReadWrite);
        return static_pointer_cast<IO::IStream>(stream);
    }

    AssetType AssetProviderDev::GetAssetType(const AssetID& assetID)
    {
        return m_Registry->GetAssetType(assetID);
    }
} // namespace FE::Assets
