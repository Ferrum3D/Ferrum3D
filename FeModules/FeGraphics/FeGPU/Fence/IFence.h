#pragma once
#include <FeCore/Memory/Memory.h>

namespace FE::GPU
{
    class IFence
    {
    public:
        virtual ~IFence() = default;
        virtual void Wait(UInt64 value) = 0;
        virtual void Signal(UInt64 value) = 0;
    };
}
