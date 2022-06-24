#include <OsGPU/Device/VKDevice.h>
#include <OsGPU/Image/VKImage.h>
#include <OsGPU/ImageView/VKImageView.h>
#include <OsGPU/Image/ImageSubresource.h>

namespace FE::GPU
{
    VKImage::VKImage(VKDevice& dev)
        : m_Device(&dev)
    {
    }

    const ImageDesc& VKImage::GetDesc()
    {
        return Desc;
    }

    Shared<IImageView> VKImage::CreateRenderTargetView()
    {
        ImageSubresourceRange range{};
        range.ArraySliceCount = Desc.ArraySize;
        range.MinArraySlice   = 0;
        range.MinMipSlice     = 0;
        range.MipSliceCount   = 1;
        range.AspectFlags     = ImageAspectFlags::Color;

        ImageViewDesc desc{};
        desc.Format           = Desc.ImageFormat;
        desc.Image            = this;
        desc.Dimension        = Desc.Dimension;
        desc.SubresourceRange = range;
        return m_Device->CreateImageView(desc);
    }

    VKImage::VKImage(VKDevice&, const ImageDesc&)
    {
        FE_UNREACHABLE("Not implemented");
    }
} // namespace FE::GPU
