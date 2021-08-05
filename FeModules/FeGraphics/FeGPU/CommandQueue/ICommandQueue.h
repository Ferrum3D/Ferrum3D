#pragma once
#include <FeGPU/Fence/IFence.h>

namespace FE::GPU
{
    class ICommandBuffer;

    class ICommandQueue
    {
    public:
        virtual ~ICommandQueue() = default;

        virtual void WaitForFence(const RefCountPtr<IFence>& fence, uint64_t value) = 0;
        virtual void SignalFence(const RefCountPtr<IFence>& fence, uint64_t value) = 0;
        virtual void SubmitBuffers(const Vector<RefCountPtr<ICommandBuffer>>& buffers) = 0;
    };
}
