#pragma once
#include <FeGPU/Device/IDevice.h>

namespace FE::GPU
{
    class VKAdapter;

    struct VKQueueFamilyData
    {
        uint32_t FamilyIndex;
        uint32_t QueueCount;
        CommandListClass Class;
        vk::UniqueCommandPool CmdPool;

        inline VKQueueFamilyData(uint32_t idx, uint32_t count, CommandListClass cmdListClass)
            : FamilyIndex(idx)
            , QueueCount(count)
            , Class(cmdListClass)
        {
        }
    };

    class VKDevice : public IDevice
    {
        vk::UniqueDevice m_NativeDevice;
        vk::PhysicalDevice* m_NativeAdapter;
        VKAdapter* m_Adapter;
        Vector<VKQueueFamilyData> m_QueueFamilyIndices;

        void FindQueueFamilies();

    public:
        VKDevice(VKAdapter& adapter);
        vk::Device GetNativeDevice();

        virtual RefCountPtr<IFence> CreateFence(uint64_t value) override;
    };
} // namespace FE::GPU
