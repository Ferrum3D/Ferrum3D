#pragma once
#include <FeCore/Base/Base.h>

namespace FE::GPU
{
    struct WindowDescBinding
    {
        UInt32 Width;
        UInt32 Height;

        const char* Title;
    };
} // namespace FE::GPU
