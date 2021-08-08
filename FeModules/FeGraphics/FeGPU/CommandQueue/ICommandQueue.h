#pragma once
#include <FeCore/Memory/Object.h>
#include <FeGPU/Fence/IFence.h>

namespace FE::GPU
{
    class ICommandBuffer;

    class ICommandQueue : public IObject
    {
    public:
        virtual ~ICommandQueue() = default;

        virtual void WaitForFence(const RefCountPtr<IFence>& fence, UInt64 value)      = 0;
        virtual void SignalFence(const RefCountPtr<IFence>& fence, UInt64 value)       = 0;
        virtual void SubmitBuffers(const Vector<RefCountPtr<ICommandBuffer>>& buffers) = 0;
    };
} // namespace FE::GPU
