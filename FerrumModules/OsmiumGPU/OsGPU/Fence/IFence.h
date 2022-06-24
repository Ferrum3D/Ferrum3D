#pragma once
#include <FeCore/Memory/Object.h>
#include <OsGPU/Fence/FenceState.h>

namespace FE::Osmium
{
    class IFence : public IObject
    {
    public:
        FE_CLASS_RTTI(IFence, "D815152F-A41F-45C8-81AB-F921F19E8AA3");

        ~IFence() override = default;

        virtual void SignalOnCPU()    = 0;
        virtual void WaitOnCPU()      = 0;
        virtual void Reset()          = 0;
        virtual FenceState GetState() = 0;
    };
} // namespace FE::Osmium
