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
        m_imageView.Reset();
        m_image.Reset();
    }
} // namespace FE::Graphics
