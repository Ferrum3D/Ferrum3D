#pragma once
#include <HAL/ImageEnums.h>

namespace FE::Graphics::HAL
{
    struct alignas(uint64_t) ImageSubresource
    {
        uint16_t MipSlice = 0;
        uint16_t ArraySlice = 0;

        ImageAspect Aspect = ImageAspect::Color;
    };


    struct alignas(uint64_t) ImageSubresourceRange
    {
        uint8_t MinMipSlice = 0;
        uint8_t MipSliceCount = 1;
        uint16_t MinArraySlice = 0;
        uint16_t ArraySliceCount = 1;

        ImageAspectFlags AspectFlags = ImageAspectFlags::All;

        inline ImageSubresourceRange() = default;

        inline ImageSubresourceRange(const ImageSubresource& subresource)
        {
            AspectFlags = static_cast<ImageAspectFlags>(1 << enum_cast(subresource.Aspect));
            MinMipSlice = static_cast<uint8_t>(subresource.MipSlice);
            MinArraySlice = subresource.ArraySlice;
            MipSliceCount = 1;
            ArraySliceCount = 1;
        }
    };
} // namespace FE::Graphics::HAL
