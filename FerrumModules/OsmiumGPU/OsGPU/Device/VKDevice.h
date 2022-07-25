#pragma once
#include <FeCore/Console/FeLog.h>
#include <FeCore/Containers/ArraySlice.h>
#include <FeCore/Containers/LRUCacheMap.h>
#include <OsGPU/Common/VKConfig.h>
#include <OsGPU/Descriptors/DescriptorDesc.h>
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

    class DescriptorSetLayoutData final
    {
        VkDescriptorSetLayout m_SetLayout;
        UInt32 m_RefCount;

    public:
        FE_STRUCT_RTTI(DescriptorSetLayoutData, "93EA161C-0BC6-45F6-962B-8A21259DFE2B");

        inline DescriptorSetLayoutData() = default;

        inline DescriptorSetLayoutData(VkDescriptorSetLayout layout)
            : m_SetLayout(layout)
            , m_RefCount(1)
        {
        }

        [[nodiscard]] inline VkDescriptorSetLayout SetLayout()
        {
            ++m_RefCount;
            return m_SetLayout;
        }

        [[nodiscard]] inline bool Release(VkDevice device)
        {
            if (--m_RefCount == 0)
            {
                vkDestroyDescriptorSetLayout(device, m_SetLayout, VK_NULL_HANDLE);
                return true;
            }

            return false;
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

        UnorderedMap<USize, DescriptorSetLayoutData> m_DescriptorSetLayouts;
        LRUCacheMap<USize, VkMemoryRequirements> m_ImageMemoryRequirementsByDesc;
        VkMemoryRequirements m_ImageMemoryRequirements;
        VkMemoryRequirements m_RenderTargetMemoryRequirements;
        VkMemoryRequirements m_BufferMemoryRequirements;

        void FindQueueFamilies();

    public:
        FE_CLASS_RTTI(VKDevice, "7AE4B802-75AF-439E-AA48-BC72761B7B72");

        explicit VKDevice(VKAdapter& adapter);
        ~VKDevice() override;
        VkDevice GetNativeDevice();

        UInt32 FindMemoryType(UInt32 typeBits, VkMemoryPropertyFlags properties);

        VkDescriptorSetLayout GetDescriptorSetLayout(const ArraySlice<DescriptorDesc>& descriptors, USize& key);
        void ReleaseDescriptorSetLayout(USize key);

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

        VkMemoryRequirements GetImageMemoryRequirements(const ImageDesc& desc);
        VkMemoryRequirements GetImageMemoryRequirements();
        VkMemoryRequirements GetRenderTargetMemoryRequirements();
        VkMemoryRequirements GetBufferMemoryRequirements();

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
        Shared<IBuffer> CreateBuffer(const BufferDesc& desc) override;
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
        Shared<ITransientResourceHeap> CreateTransientResourceHeap(const TransientResourceHeapDesc& desc) override;
    };
} // namespace FE::Osmium
