#pragma once
#include <FeGPU/CommandBuffer/ICommandBuffer.h>
#include <FeGPU/CommandQueue/ICommandQueue.h>
#include <FeGPU/Fence/IFence.h>
#include <FeGPU/Resource/IResource.h>
#include <FeGPU/SwapChain/ISwapChain.h>
#include <FeGPU/Buffer/IBuffer.h>

namespace FE::GPU
{
    enum class CommandQueueClass
    {
        Graphics,
        Compute,
        Transfer
    };

    class IInstance;
    class IAdapter;

    class IDevice
    {
    public:
        virtual ~IDevice() = default;

        virtual IAdapter& GetAdapter()                                                           = 0;
        virtual IInstance& GetInstance()                                                         = 0;
        virtual RefCountPtr<IFence> CreateFence(UInt64 value)                                  = 0;
        virtual RefCountPtr<ICommandQueue> GetCommandQueue(CommandQueueClass cmdQueueClass)      = 0;
        virtual RefCountPtr<ICommandBuffer> CreateCommandBuffer(CommandQueueClass cmdQueueClass) = 0;
        virtual RefCountPtr<ISwapChain> CreateSwapChain(const SwapChainDesc& desc)               = 0;
        virtual RefCountPtr<IBuffer> CreateBuffer(BindFlags bindFlags, UInt64 size)            = 0;
    };
} // namespace FE::GPU
