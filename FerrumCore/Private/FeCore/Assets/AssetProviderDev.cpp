#include <FeCore/Assets/AssetProviderDev.h>
#include <FeCore/IO/FileStream.h>

namespace FE::Assets
{
    void AssetProviderDev::AttachRegistry(const Rc<AssetRegistry>& registry)
    {
        m_Registry = registry;
    }

    void AssetProviderDev::DetachRegistry()
    {
        m_Registry.Reset();
    }

    Rc<IO::IStream> AssetProviderDev::CreateAssetLoadingStream(const AssetID& assetID)
    {
        FE_ASSERT_MSG(m_Registry, "Registry was not attached");
        auto fileName = m_Registry->GetAssetFilePath(assetID);
        Rc stream = Rc<IO::FileStream>::DefaultNew(Rc<IO::FileHandle>::DefaultNew());
        FE_IO_ASSERT(stream->Open(fileName, IO::OpenMode::ReadWrite));
        return static_pointer_cast<IO::IStream>(stream);
    }

    AssetType AssetProviderDev::GetAssetType(const AssetID& assetID)
    {
        return m_Registry->GetAssetType(assetID);
    }
} // namespace FE::Assets
