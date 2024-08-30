#pragma once
#include <HAL/DeviceObject.h>

namespace FE::Graphics::HAL
{
    enum class SubmitFlags
    {
        None = 0,
        FrameBegin = 1 << 0,
        FrameEnd = 1 << 1,
        FrameBeginEnd = FrameBegin | FrameEnd
    };

    FE_ENUM_OPERATORS(SubmitFlags);

    class CommandList;
    class Fence;

    class CommandQueue : public DeviceObject
    {
    public:
        ~CommandQueue() override = default;

        FE_RTTI_Class(CommandQueue, "2BC9A588-BF3E-420B-A8C7-6DC770E5F4B3");

        virtual void SignalFence(Fence* fence) = 0;

        virtual void SubmitBuffers(festd::span<CommandList* const> commandLists, Fence* signalFence, SubmitFlags flags) = 0;
    };
} // namespace FE::Graphics::HAL
