#pragma once
#include <FeGPU/Common/VKConfig.h>
#include <FeCore/Console/FeLog.h>
#include <FeGPU/Device/IDevice.h>
#include <FeGPU/Instance/IInstance.h>
#include <FeGPU/CommandBuffer/VKCommandBuffer.h>

namespace FE::GPU
{
    class VKAdapter;

    struct VKQueueFamilyData
    {
        UInt32 FamilyIndex;
        UInt32 QueueCount;
        CommandQueueClass Class;
        vk::UniqueCommandPool CmdPool;

        FE_STRUCT_RTTI(VKQueueFamilyData, "95E71464-EA4F-42C3-8838-881FCE46754D");

        inline VKQueueFamilyData(UInt32 idx, UInt32 count, CommandQueueClass cmdListClass)
            : FamilyIndex(idx)
            , QueueCount(count)
            , Class(cmdListClass)
        {
        }
    };

    class VKInstance;

    class VKDevice : public Object<IDevice>
    {
        vk::UniqueDevice m_NativeDevice;
        vk::PhysicalDevice* m_NativeAdapter;
        VKAdapter* m_Adapter;
        VKInstance* m_Instance;
        Vector<VKQueueFamilyData> m_QueueFamilyIndices;

        void FindQueueFamilies();

    public:
        FE_CLASS_RTTI(VKDevice, "7AE4B802-75AF-439E-AA48-BC72761B7B72");

        explicit VKDevice(VKAdapter& adapter);
        vk::Device& GetNativeDevice();

        UInt32 FindMemoryType(UInt32 typeBits, vk::MemoryPropertyFlags properties);

        inline vk::CommandPool& GetCommandPool(CommandQueueClass cmdQueueClass)
        {
            for (auto& queue : m_QueueFamilyIndices)
            {
                if (queue.Class == cmdQueueClass)
                {
                    return queue.CmdPool.get();
                }
            }

            FE_UNREACHABLE("Couldn't find command pool");
            return m_QueueFamilyIndices.front().CmdPool.get();
        }

        inline vk::CommandPool& GetCommandPool(UInt32 queueFamilyIndex)
        {
            for (auto& queue : m_QueueFamilyIndices)
            {
                if (queue.FamilyIndex == queueFamilyIndex)
                {
                    return queue.CmdPool.get();
                }
            }

            FE_UNREACHABLE("Couldn't find command pool");
            return m_QueueFamilyIndices.front().CmdPool.get();
        }

        inline UInt32 GetQueueFamilyIndex(CommandQueueClass cmdQueueClass)
        {
            for (auto& queue : m_QueueFamilyIndices)
            {
                if (queue.Class == cmdQueueClass)
                {
                    return queue.FamilyIndex;
                }
            }

            FE_UNREACHABLE("Couldn't find queue family");
            return static_cast<UInt32>(-1);
        }

        void WaitIdle() override;
        IAdapter& GetAdapter() override;
        IInstance& GetInstance() override;
        RefCountPtr<IFence> CreateFence(FenceState state) override;
        RefCountPtr<ICommandQueue> GetCommandQueue(CommandQueueClass cmdQueueClass) override;
        RefCountPtr<ICommandBuffer> CreateCommandBuffer(CommandQueueClass cmdQueueClass) override;
        RefCountPtr<ISwapChain> CreateSwapChain(const SwapChainDesc& desc) override;
        RefCountPtr<IBuffer> CreateBuffer(BindFlags bindFlags, UInt64 size) override;
        RefCountPtr<IShaderModule> CreateShaderModule(const ShaderModuleDesc& desc) override;
        RefCountPtr<IRenderPass> CreateRenderPass(const RenderPassDesc& desc) override;
        RefCountPtr<IDescriptorHeap> CreateDescriptorHeap(const DescriptorHeapDesc& desc) override;
        RefCountPtr<VKCommandBuffer> CreateCommandBuffer(UInt32 queueFamilyIndex);
        RefCountPtr<IShaderCompiler> CreateShaderCompiler() override;
        RefCountPtr<IGraphicsPipeline> CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) override;
        RefCountPtr<IImageView> CreateImageView(const ImageViewDesc& desc) override;
        RefCountPtr<IFramebuffer> CreateFramebuffer(const FramebufferDesc& desc) override;
    };
} // namespace FE::GPU
