#pragma once
#include <OsGPU/Image/ImageEnums.h>

namespace FE::Osmium
{
    struct ImageSubresource
    {
        UInt16 MipSlice   = 0;
        UInt16 ArraySlice = 0;

        ImageAspect Aspect = ImageAspect::Color;

        FE_STRUCT_RTTI(ImageSubresource, "A4E67F19-16D0-4C1C-BD8B-D270A4141D45");
    };

    struct ImageSubresourceRange
    {
        UInt16 MinMipSlice     = 0;
        UInt16 MinArraySlice   = 0;
        UInt16 MipSliceCount   = 1;
        UInt16 ArraySliceCount = 1;

        ImageAspectFlags AspectFlags = ImageAspectFlags::All;

        FE_STRUCT_RTTI(ImageSubresourceRange, "93D87B09-78B5-40C9-8104-F06699FA6D10");

        inline ImageSubresourceRange() = default;

        inline ImageSubresourceRange(const ImageSubresource& subresource) // NOLINT(google-explicit-constructor)
        {
            AspectFlags     = static_cast<ImageAspectFlags>(1 << static_cast<UInt32>(subresource.Aspect));
            MinMipSlice     = subresource.MipSlice;
            MinArraySlice   = subresource.ArraySlice;
            MipSliceCount   = 1;
            ArraySliceCount = 1;
        }
    };
} // namespace FE::Osmium
