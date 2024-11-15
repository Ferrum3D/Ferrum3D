#pragma once
#include <HAL/DeviceObject.h>

namespace FE::Graphics::HAL
{
    class CommandList;
    struct FenceSyncPoint;

    struct CommandQueue : public DeviceObject
    {
        ~CommandQueue() override = default;

        FE_RTTI_Class(CommandQueue, "2BC9A588-BF3E-420B-A8C7-6DC770E5F4B3");

        virtual void SignalFence(const FenceSyncPoint& fence) = 0;
        virtual void WaitFence(const FenceSyncPoint& fence) = 0;

        virtual void Execute(festd::span<CommandList* const> commandLists) = 0;
    };
} // namespace FE::Graphics::HAL
