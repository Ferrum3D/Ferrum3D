#include <Graphics/Assets/ImageAssetLoader.h>
#include <Graphics/Assets/ImageAssetStorage.h>

namespace FE::Graphics
{
    ImageAssetStorage::ImageAssetStorage(ImageAssetLoader* loader)
        : Assets::AssetStorage(loader)
    {
    }


    void ImageAssetStorage::Delete()
    {
        m_ImageView.Reset();
        m_Image.Reset();
    }
} // namespace FE::Graphics
