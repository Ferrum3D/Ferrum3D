#pragma once
#include <FeCore/Console/FeLog.h>
#include <OsGPU/Common/VKConfig.h>
#include <OsGPU/Device/IDevice.h>

namespace FE::Osmium
{
    class VKAdapter;

    struct VKQueueFamilyData
    {
        UInt32 FamilyIndex;
        UInt32 QueueCount;
        CommandQueueClass Class;
        VkCommandPool CmdPool;

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
        VkDevice m_NativeDevice;
        VkPhysicalDevice m_NativeAdapter;
        VKAdapter* m_Adapter;
        VKInstance* m_Instance;
        List<VKQueueFamilyData> m_QueueFamilyIndices;

        List<VkSemaphore> m_WaitSemaphores;
        List<VkSemaphore> m_SignalSemaphores;

        void FindQueueFamilies();

    public:
        FE_CLASS_RTTI(VKDevice, "7AE4B802-75AF-439E-AA48-BC72761B7B72");

        explicit VKDevice(VKAdapter& adapter);
        ~VKDevice() override;
        VkDevice GetNativeDevice();

        UInt32 FindMemoryType(UInt32 typeBits, VkMemoryPropertyFlags properties);

        inline VkCommandPool GetCommandPool(CommandQueueClass cmdQueueClass)
        {
            for (auto& queue : m_QueueFamilyIndices)
            {
                if (queue.Class == cmdQueueClass)
                {
                    return queue.CmdPool;
                }
            }

            FE_UNREACHABLE("Couldn't find command pool");
            return m_QueueFamilyIndices.Front().CmdPool;
        }

        inline VkCommandPool GetCommandPool(UInt32 queueFamilyIndex)
        {
            for (auto& queue : m_QueueFamilyIndices)
            {
                if (queue.FamilyIndex == queueFamilyIndex)
                {
                    return queue.CmdPool;
                }
            }

            FE_UNREACHABLE("Couldn't find command pool");
            return m_QueueFamilyIndices.Front().CmdPool;
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

        VkSemaphore& AddWaitSemaphore();
        VkSemaphore& AddSignalSemaphore();
        UInt32 GetWaitSemaphores(const VkSemaphore** semaphores);
        UInt32 GetSignalSemaphores(const VkSemaphore** semaphores);

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
        Shared<ISampler> CreateSampler(const SamplerDesc& desc) override;
    };
} // namespace FE::Osmium
