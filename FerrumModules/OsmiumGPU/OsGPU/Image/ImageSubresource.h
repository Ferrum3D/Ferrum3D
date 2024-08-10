#pragma once
#include <OsGPU/Image/ImageEnums.h>

namespace FE::Osmium
{
    struct ImageSubresource
    {
        uint16_t MipSlice   = 0;
        uint16_t ArraySlice = 0;

        ImageAspect Aspect = ImageAspect::Color;

        FE_RTTI_Base(ImageSubresource, "A4E67F19-16D0-4C1C-BD8B-D270A4141D45");
    };

    struct ImageSubresourceRange
    {
        uint16_t MinMipSlice     = 0;
        uint16_t MinArraySlice   = 0;
        uint16_t MipSliceCount   = 1;
        uint16_t ArraySliceCount = 1;

        ImageAspectFlags AspectFlags = ImageAspectFlags::All;

        FE_RTTI_Base(ImageSubresourceRange, "93D87B09-78B5-40C9-8104-F06699FA6D10");

        inline ImageSubresourceRange() = default;

        inline ImageSubresourceRange(const ImageSubresource& subresource) // NOLINT(google-explicit-constructor)
        {
            AspectFlags     = static_cast<ImageAspectFlags>(1 << static_cast<uint32_t>(subresource.Aspect));
            MinMipSlice     = subresource.MipSlice;
            MinArraySlice   = subresource.ArraySlice;
            MipSliceCount   = 1;
            ArraySliceCount = 1;
        }
    };
} // namespace FE::Osmium
