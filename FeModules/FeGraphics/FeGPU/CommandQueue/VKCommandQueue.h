#pragma once
#include <FeGPU/CommandQueue/ICommandQueue.h>
#include <FeGPU/Common/VKConfig.h>

namespace FE::GPU
{
    class VKDevice;

    struct VKCommandQueueDesc
    {
        FE_STRUCT_RTTI(VKCommandQueueDesc, "3DC339A9-9E9A-48C4-960E-1048B6939D1E");

        UInt32 QueueFamilyIndex;
        UInt32 QueueIndex;
    };

    class VKCommandQueue : public Object<ICommandQueue>
    {
        VKDevice* m_Device;
        vk::Queue m_Queue;
        VKCommandQueueDesc m_Desc;

    public:
        FE_CLASS_RTTI(VKCommandQueue, "416B9666-BFB4-4DB6-85C8-1AB6D5A318C5");

        VKCommandQueue(VKDevice& dev, const VKCommandQueueDesc& desc);

        virtual void WaitForFence(const RefCountPtr<IFence>& fence, UInt64 value) override;
        virtual void SignalFence(const RefCountPtr<IFence>& fence, UInt64 value) override;
        virtual void SubmitBuffers(const Vector<RefCountPtr<ICommandBuffer>>& buffers) override;

        const VKCommandQueueDesc& GetDesc() const;
        vk::Queue GetNativeQueue();
    };
}
