#pragma once
#include <FeCore/Memory/Memory.h>

namespace FE::GPU
{
    class IFence
    {
    public:
        virtual ~IFence() = default;
        virtual void Wait(uint64_t value) = 0;
        virtual void Signal(uint64_t value) = 0;
    };
}
