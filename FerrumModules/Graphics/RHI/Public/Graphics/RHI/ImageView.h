#pragma once
#include <Graphics/RHI/DeviceObject.h>
#include <Graphics/RHI/Image.h>

namespace FE::Graphics::RHI
{
    struct ImageViewDesc final
    {
        Image* m_image = nullptr;
        ImageSubresourceRange m_subresourceRange;
        Format m_format = Format::kUndefined;
        ImageDimension m_dimension = ImageDimension::k2D;

        static ImageViewDesc ForImage(Image* image, RHI::ImageAspectFlags aspectFlags)
        {
            const ImageDesc& imageDesc = image->GetDesc();
            ImageSubresourceRange range{};
            range.m_arraySliceCount = imageDesc.m_arraySize;
            range.m_minArraySlice = 0;
            range.m_minMipSlice = 0;
            range.m_mipSliceCount = imageDesc.m_mipSliceCount;
            range.m_aspectFlags = aspectFlags;

            ImageViewDesc desc{};
            desc.m_format = imageDesc.m_imageFormat;
            desc.m_image = image;
            desc.m_dimension = imageDesc.m_dimension;
            desc.m_subresourceRange = range;
            return desc;
        }
    };


    struct ImageView : public DeviceObject
    {
        FE_RTTI_Class(ImageView, "16C72764-BC1D-4745-A83E-51D021ACA35D");

        ~ImageView() override = default;

        virtual ResultCode Init(const ImageViewDesc& desc) = 0;

        [[nodiscard]] virtual const ImageViewDesc& GetDesc() const = 0;
    };
} // namespace FE::Graphics::RHI
