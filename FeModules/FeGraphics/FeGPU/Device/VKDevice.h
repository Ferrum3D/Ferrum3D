#pragma once
#include <FeGPU/Device/IDevice.h>
#include <FeCore/Console/FeLog.h>
#include <FeGPU/Instance/IInstance.h>

namespace FE::GPU
{
    class VKAdapter;

    struct VKQueueFamilyData
    {
        uint32_t FamilyIndex;
        uint32_t QueueCount;
        CommandQueueClass Class;
        vk::UniqueCommandPool CmdPool;

        inline VKQueueFamilyData(uint32_t idx, uint32_t count, CommandQueueClass cmdListClass)
            : FamilyIndex(idx)
            , QueueCount(count)
            , Class(cmdListClass)
        {
        }
    };

    class VKInstance;

    class VKDevice : public IDevice
    {
        vk::UniqueDevice m_NativeDevice;
        vk::PhysicalDevice* m_NativeAdapter;
        VKAdapter* m_Adapter;
        VKInstance* m_Instance;
        Vector<VKQueueFamilyData> m_QueueFamilyIndices;

        void FindQueueFamilies();

    public:
        VKDevice(VKAdapter& adapter);
        vk::Device& GetNativeDevice();

        inline vk::CommandPool& GetCommandPool(CommandQueueClass cmdQueueClass)
        {
            for (auto& queue : m_QueueFamilyIndices)
                if (queue.Class == cmdQueueClass)
                    return queue.CmdPool.get();
            FE_UNREACHABLE("Couldn't find command pool");
            return m_QueueFamilyIndices.front().CmdPool.get();
        }

        inline uint32_t GetQueueFamilyIndex(CommandQueueClass cmdQueueClass)
        {
            for (auto& queue : m_QueueFamilyIndices)
                if (queue.Class == cmdQueueClass)
                    return queue.FamilyIndex;
            FE_UNREACHABLE("Couldn't find queue family");
            return -1;
        }

        virtual IAdapter& GetAdapter() override;
        virtual IInstance& GetInstance() override;
        virtual RefCountPtr<IFence> CreateFence(uint64_t value) override;
        virtual RefCountPtr<ICommandQueue> GetCommandQueue(CommandQueueClass cmdQueueClass) override;
        virtual RefCountPtr<ICommandBuffer> CreateCommandBuffer(CommandQueueClass cmdQueueClass) override;
    };
} // namespace FE::GPU
