#include <OsAssets/Images/ImageAssetLoader.h>
#include <OsAssets/Images/ImageAssetStorage.h>
#include <OsAssets/Images/StbImage.h>

namespace FE::Osmium
{
    ImageAssetStorage::ImageAssetStorage(ImageAssetLoader* loader)
        : Assets::AssetStorage(loader)
    {
    }

    void ImageAssetStorage::Delete()
    {
        if (m_Data)
        {
            Internal::FreeImageMemory(m_Data);
            m_Data = nullptr;
        }
    }
} // namespace FE::Osmium
