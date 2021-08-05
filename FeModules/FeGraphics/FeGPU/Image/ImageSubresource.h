#pragma once
#include <FeGPU/Image/ImageEnums.h>

namespace FE::GPU
{
    struct ImageSubresource
    {
        uint16_t MipSlice   = 0;
        uint16_t ArraySlice = 0;
        ImageAspect Apsect  = ImageAspect::Color;
    };

    struct ImageSubresourceRange
    {
        uint16_t MinMipSlice     = 0;
        uint16_t MinArraySlice   = 0;
        uint16_t MipSliceCount   = 1;
        uint16_t ArraySliceCount = 1;
        ImageAspect Apsect       = ImageAspect::All;
    };
} // namespace FE::GPU
