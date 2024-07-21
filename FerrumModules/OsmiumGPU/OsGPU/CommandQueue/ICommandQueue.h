#pragma once
#include <FeCore/Containers/ArraySlice.h>

namespace FE::Osmium
{
    enum class SubmitFlags
    {
        None = 0,
        FrameBegin = 1 << 0,
        FrameEnd = 1 << 1,
        FrameBeginEnd = FrameBegin | FrameEnd
    };

    FE_ENUM_OPERATORS(SubmitFlags);

    class ICommandBuffer;
    class IFence;

    class ICommandQueue : public Memory::RefCountedObjectBase
    {
    public:
        ~ICommandQueue() override = default;

        FE_CLASS_RTTI(ICommandQueue, "2BC9A588-BF3E-420B-A8C7-6DC770E5F4B3");

        virtual void SignalFence(IFence* fence) = 0;

        virtual void SubmitBuffers(const ArraySlice<ICommandBuffer*>& buffers, IFence* signalFence, SubmitFlags flags) = 0;
    };
} // namespace FE::Osmium
