#pragma once
#include <HAL/DeviceObject.h>

namespace FE::Graphics::HAL
{
    class Fence : public DeviceObject
    {
    public:
        FE_RTTI_Class(Fence, "D815152F-A41F-45C8-81AB-F921F19E8AA3");

        ~Fence() override = default;

        virtual ResultCode Init(FenceState initialState) = 0;

        virtual void SignalOnCPU() = 0;
        virtual void WaitOnCPU() = 0;
        virtual void Reset() = 0;
        virtual FenceState GetState() = 0;
    };
} // namespace FE::Graphics::HAL
