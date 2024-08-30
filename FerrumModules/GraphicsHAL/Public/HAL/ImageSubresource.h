#pragma once
#include <HAL/ImageEnums.h>

namespace FE::Graphics::HAL
{
    struct ImageSubresource
    {
        ImageAspect Aspect = ImageAspect::kColor;
        uint8_t MipSlice = 0;
        uint16_t ArraySlice = 0;
    };


    struct alignas(uint64_t) ImageSubresourceRange
    {
        uint8_t MinMipSlice = 0;
        uint8_t MipSliceCount = 1;
        uint16_t MinArraySlice = 0;
        uint16_t ArraySliceCount = 1;

        ImageAspectFlags AspectFlags = ImageAspectFlags::kAll;

        inline ImageSubresourceRange() = default;

        inline ImageSubresourceRange(const ImageSubresource& subresource)
        {
            AspectFlags = static_cast<ImageAspectFlags>(1 << enum_cast(subresource.Aspect));
            MinMipSlice = subresource.MipSlice;
            MinArraySlice = subresource.ArraySlice;
            MipSliceCount = 1;
            ArraySliceCount = 1;
        }
    };
} // namespace FE::Graphics::HAL
