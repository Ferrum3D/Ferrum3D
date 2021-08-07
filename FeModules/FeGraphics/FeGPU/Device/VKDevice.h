#pragma once
#include <FeCore/Console/FeLog.h>
#include <FeGPU/Device/IDevice.h>
#include <FeGPU/Instance/IInstance.h>

namespace FE::GPU
{
    class VKAdapter;

    struct VKQueueFamilyData
    {
        UInt32 FamilyIndex;
        UInt32 QueueCount;
        CommandQueueClass Class;
        vk::UniqueCommandPool CmdPool;

        inline VKQueueFamilyData(UInt32 idx, UInt32 count, CommandQueueClass cmdListClass)
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

        UInt32 FindMemoryType(UInt32 typeBits, vk::MemoryPropertyFlags properties);

        inline vk::CommandPool& GetCommandPool(CommandQueueClass cmdQueueClass)
        {
            for (auto& queue : m_QueueFamilyIndices)
                if (queue.Class == cmdQueueClass)
                    return queue.CmdPool.get();
            FE_UNREACHABLE("Couldn't find command pool");
            return m_QueueFamilyIndices.front().CmdPool.get();
        }

        inline UInt32 GetQueueFamilyIndex(CommandQueueClass cmdQueueClass)
        {
            for (auto& queue : m_QueueFamilyIndices)
                if (queue.Class == cmdQueueClass)
                    return queue.FamilyIndex;
            FE_UNREACHABLE("Couldn't find queue family");
            return static_cast<UInt32>(-1);
        }

        virtual IAdapter& GetAdapter() override;
        virtual IInstance& GetInstance() override;
        virtual RefCountPtr<IFence> CreateFence(UInt64 value) override;
        virtual RefCountPtr<ICommandQueue> GetCommandQueue(CommandQueueClass cmdQueueClass) override;
        virtual RefCountPtr<ICommandBuffer> CreateCommandBuffer(CommandQueueClass cmdQueueClass) override;
        virtual RefCountPtr<ISwapChain> CreateSwapChain(const SwapChainDesc& desc) override;
        virtual RefCountPtr<IBuffer> CreateBuffer(BindFlags bindFlags, UInt64 size) override;
    };
} // namespace FE::GPU
