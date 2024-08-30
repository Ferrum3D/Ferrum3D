#pragma once
#include <HAL/DeviceObject.h>
#include <HAL/Image.h>

namespace FE::Graphics::HAL
{
    struct ImageViewDesc
    {
        HAL::Image* Image = nullptr;
        ImageSubresourceRange SubresourceRange;
        Format Format = Format::kUndefined;
        ImageDim Dimension = ImageDim::kImage2D;

        inline static ImageViewDesc ForImage(HAL::Image* image, HAL::ImageAspectFlags aspectFlags)
        {
            const ImageDesc& imageDesc = image->GetDesc();
            ImageSubresourceRange range{};
            range.ArraySliceCount = imageDesc.ArraySize;
            range.MinArraySlice = 0;
            range.MinMipSlice = 0;
            range.MipSliceCount = static_cast<uint8_t>(imageDesc.MipSliceCount);
            range.AspectFlags = aspectFlags;

            ImageViewDesc desc{};
            desc.Format = imageDesc.ImageFormat;
            desc.Image = image;
            desc.Dimension = imageDesc.GetDimension();
            desc.SubresourceRange = range;
            return desc;
        }
    };


    class ImageView : public DeviceObject
    {
    public:
        FE_RTTI_Class(ImageView, "16C72764-BC1D-4745-A83E-51D021ACA35D");

        ~ImageView() override = default;

        virtual ResultCode Init(const ImageViewDesc& desc) = 0;

        [[nodiscard]] virtual const ImageViewDesc& GetDesc() const = 0;
    };
} // namespace FE::Graphics::HAL
