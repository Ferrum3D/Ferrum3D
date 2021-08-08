#pragma once
#include <FeGPU/CommandQueue/ICommandQueue.h>
#include <FeGPU/Common/VKConfig.h>

namespace FE::GPU
{
    class VKDevice;

    struct VKCommandQueueDesc
    {
        UInt32 QueueFamilyIndex;
        UInt32 QueueIndex;
    };

    class VKCommandQueue : public Object<ICommandQueue>
    {
        VKDevice* m_Device;
        vk::Queue m_Queue;
        VKCommandQueueDesc m_Desc;

    public:
        VKCommandQueue(VKDevice& dev, const VKCommandQueueDesc& desc);

        virtual void WaitForFence(const RefCountPtr<IFence>& fence, UInt64 value) override;
        virtual void SignalFence(const RefCountPtr<IFence>& fence, UInt64 value) override;
        virtual void SubmitBuffers(const Vector<RefCountPtr<ICommandBuffer>>& buffers) override;

        const VKCommandQueueDesc& GetDesc() const;
        vk::Queue GetNativeQueue();
    };
}
