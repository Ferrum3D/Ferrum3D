#pragma once
#include <FeCore/Console/FeLog.h>
#include <OsGPU/Common/VKConfig.h>
#include <OsGPU/Device/IDevice.h>

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
    class VKCommandBuffer;

    class VKDevice final : public Object<IDevice>
    {
        vk::UniqueDevice m_NativeDevice;
        vk::PhysicalDevice* m_NativeAdapter;
        VKAdapter* m_Adapter;
        VKInstance* m_Instance;
        Vector<VKQueueFamilyData> m_QueueFamilyIndices;

        Vector<vk::Semaphore> m_WaitSemaphores;
        Vector<vk::Semaphore> m_SignalSemaphores;

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

        vk::Semaphore& AddWaitSemaphore();
        vk::Semaphore& AddSignalSemaphore();
        UInt32 GetWaitSemaphores(const vk::Semaphore** semaphores);
        UInt32 GetSignalSemaphores(const vk::Semaphore** semaphores);

        void WaitIdle() override;
        IAdapter& GetAdapter() override;
        IInstance& GetInstance() override;
        Shared<IFence> CreateFence(FenceState state) override;
        Shared<ICommandQueue> GetCommandQueue(CommandQueueClass cmdQueueClass) override;
        Shared<ICommandBuffer> CreateCommandBuffer(CommandQueueClass cmdQueueClass) override;
        Shared<ISwapChain> CreateSwapChain(const SwapChainDesc& desc) override;
        Shared<IBuffer> CreateBuffer(BindFlags bindFlags, UInt64 size) override;
        Shared<IShaderModule> CreateShaderModule(const ShaderModuleDesc& desc) override;
        Shared<IRenderPass> CreateRenderPass(const RenderPassDesc& desc) override;
        Shared<IDescriptorHeap> CreateDescriptorHeap(const DescriptorHeapDesc& desc) override;
        Shared<VKCommandBuffer> CreateCommandBuffer(UInt32 queueFamilyIndex);
        Shared<IShaderCompiler> CreateShaderCompiler() override;
        Shared<IGraphicsPipeline> CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) override;
        Shared<IImageView> CreateImageView(const ImageViewDesc& desc) override;
        Shared<IFramebuffer> CreateFramebuffer(const FramebufferDesc& desc) override;
        Shared<IWindow> CreateWindow(const WindowDesc& desc) override;
        Shared<IImage> CreateImage(const ImageDesc& desc) override;
    };
} // namespace FE::GPU
