#pragma once
#include <FeCore/Containers/List.h>
#include <FeCore/Memory/Object.h>

namespace FE::GPU
{
    enum class SubmitFlags
    {
        None          = 0,
        FrameBegin    = 1 << 0,
        FrameEnd      = 1 << 1,
        FrameBeginEnd = FrameBegin | FrameEnd
    };

    FE_ENUM_OPERATORS(SubmitFlags);

    class ICommandBuffer;
    class IFence;

    class ICommandQueue : public IObject
    {
    public:
        ~ICommandQueue() override = default;

        FE_CLASS_RTTI(ICommandQueue, "2BC9A588-BF3E-420B-A8C7-6DC770E5F4B3");

        virtual void SignalFence(const Shared<IFence>& fence) = 0;

        virtual void SubmitBuffers(
            const List<ICommandBuffer*>& buffers, const Shared<IFence>& signalFence, SubmitFlags flags) = 0;
    };
} // namespace FE::GPU
