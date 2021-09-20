#include <GPU/Device/VKDevice.h>
#include <GPU/Image/VKImage.h>
#include <GPU/Image/VKImageFormat.h>
#include <GPU/ImageView/VKImageView.h>

namespace FE::GPU
{
    inline vk::ImageViewType VKConvert(ImageDim dim, bool isArray)
    {
        switch (dim)
        {
        case ImageDim::Image1D:
            return isArray ? vk::ImageViewType::e1DArray : vk::ImageViewType::e1D;
        case ImageDim::Image2D:
            return isArray ? vk::ImageViewType::e2DArray : vk::ImageViewType::e2D;
        case ImageDim::Image3D:
            FE_ASSERT_MSG(!isArray, "Array of 3D images is not allowed");
            return vk::ImageViewType::e3D;
        case ImageDim::ImageCubemap:
            return isArray ? vk::ImageViewType::eCubeArray : vk::ImageViewType::eCube;
        default:
            FE_UNREACHABLE("Invalid ImageDim");
            return vk::ImageViewType::e2D;
        }
    }

    const ImageViewDesc& VKImageView::GetDesc() const
    {
        return m_Desc;
    }

    VKImageView::VKImageView(VKDevice& dev, const ImageViewDesc& desc)
        : m_Desc(desc)
        , m_Device(&dev)
    {
        vk::ImageViewCreateInfo viewCI{};
        viewCI.components       = vk::ComponentMapping();
        viewCI.format           = VKConvert(desc.Format);
        viewCI.image            = fe_assert_cast<VKImage*>(desc.Image.GetRaw())->Image;
        viewCI.viewType         = VKConvert(desc.Dimension, desc.SubresourceRange.ArraySliceCount != 1);
        viewCI.subresourceRange = VKConvert(desc.SubresourceRange);
        m_NativeView            = m_Device->GetNativeDevice().createImageViewUnique(viewCI);
    }
} // namespace FE::GPU
