#pragma once
#include <FeCore/Memory/Object.h>
#include <FeCore/Memory/Memory.h>

namespace FE::GPU
{
    class IFence : public IObject
    {
    public:
        FE_CLASS_RTTI(IFence, "D815152F-A41F-45C8-81AB-F921F19E8AA3");

        virtual ~IFence() = default;
        virtual void Wait(UInt64 value) = 0;
        virtual void Signal(UInt64 value) = 0;
    };
}
