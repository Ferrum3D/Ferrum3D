#pragma once
#include <GPU/Window/IWindow.h>

namespace FE::GPU
{
    struct WindowDescBinding
    {
        UInt32 Width;
        UInt32 Height;

        const char* Title;
    };
}
