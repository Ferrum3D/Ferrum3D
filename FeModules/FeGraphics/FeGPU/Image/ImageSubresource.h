#pragma once
#include <FeGPU/Image/ImageEnums.h>

namespace FE::GPU
{
    struct ImageSubresource
    {
        UInt16 MipSlice   = 0;
        UInt16 ArraySlice = 0;
        ImageAspect Apsect  = ImageAspect::Color;
    };

    struct ImageSubresourceRange
    {
        UInt16 MinMipSlice     = 0;
        UInt16 MinArraySlice   = 0;
        UInt16 MipSliceCount   = 1;
        UInt16 ArraySliceCount = 1;
        ImageAspect Apsect       = ImageAspect::All;
    };
} // namespace FE::GPU
