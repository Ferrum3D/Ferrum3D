#pragma once
#include <OsGPU/CommandQueue/ICommandQueue.h>
#include <OsGPU/Common/VKConfig.h>

namespace FE::Osmium
{
    class VKDevice;

    struct VKCommandQueueDesc
    {
        FE_STRUCT_RTTI(VKCommandQueueDesc, "3DC339A9-9E9A-48C4-960E-1048B6939D1E");

        UInt32 QueueFamilyIndex;
        UInt32 QueueIndex;
    };

    class VKCommandQueue : public ICommandQueue
    {
        VKDevice* m_Device;
        VkQueue m_Queue;
        VKCommandQueueDesc m_Desc;

    public:
        FE_CLASS_RTTI(VKCommandQueue, "416B9666-BFB4-4DB6-85C8-1AB6D5A318C5");

        VKCommandQueue(VKDevice& dev, const VKCommandQueueDesc& desc);
        ~VKCommandQueue() override = default;

        void SignalFence(IFence* fence) override;
        void SubmitBuffers(const ArraySlice<ICommandBuffer*>& buffers, IFence* signalFence, SubmitFlags flags) override;

        [[nodiscard]] const VKCommandQueueDesc& GetDesc() const;
        VkQueue GetNativeQueue();
    };
} // namespace FE::Osmium
