#pragma once
#include <FeCore/Memory/Object.h>
#include <FeCore/Memory/Memory.h>

namespace FE::GPU
{
    class IFence : public IObject
    {
    public:
        virtual ~IFence() = default;
        virtual void Wait(UInt64 value) = 0;
        virtual void Signal(UInt64 value) = 0;
    };
}
