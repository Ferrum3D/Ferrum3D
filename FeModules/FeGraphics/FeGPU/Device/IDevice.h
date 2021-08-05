#pragma once
#include <FeGPU/CommandBuffer/ICommandBuffer.h>
#include <FeGPU/CommandQueue/ICommandQueue.h>
#include <FeGPU/Fence/IFence.h>

namespace FE::GPU
{
    enum class CommandQueueClass
    {
        Graphics,
        Compute,
        Transfer
    };

    class ICommandQueue;
    class IInstance;
    class IAdapter;

    class IDevice
    {
    public:
        virtual ~IDevice() = default;

        virtual IAdapter& GetAdapter()                                                           = 0;
        virtual IInstance& GetInstance()                                                         = 0;
        virtual RefCountPtr<IFence> CreateFence(uint64_t value)                                  = 0;
        virtual RefCountPtr<ICommandQueue> GetCommandQueue(CommandQueueClass cmdQueueClass)      = 0;
        virtual RefCountPtr<ICommandBuffer> CreateCommandBuffer(CommandQueueClass cmdQueueClass) = 0;
    };
} // namespace FE::GPU
