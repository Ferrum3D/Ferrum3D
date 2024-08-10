#pragma once
#include <FeCore/Console/FeLog.h>
#include <FeCore/Containers/ArraySlice.h>
#include <FeCore/Containers/LRUCacheMap.h>
#include <FeCore/EventBus/EventBus.h>
#include <FeCore/EventBus/FrameEvents.h>
#include <OsGPU/Common/VKConfig.h>
#include <OsGPU/Descriptors/DescriptorDesc.h>
#include <OsGPU/Device/IDevice.h>

namespace FE::Osmium
{
    class VKAdapter;

    struct VKQueueFamilyData
    {
        uint32_t FamilyIndex;
        uint32_t QueueCount;
        CommandQueueClass Class;
        VkCommandPool CmdPool;

        FE_RTTI_Base(VKQueueFamilyData, "95E71464-EA4F-42C3-8838-881FCE46754D");

        inline VKQueueFamilyData(uint32_t idx, uint32_t count, CommandQueueClass cmdListClass)
            : FamilyIndex(idx)
            , QueueCount(count)
            , Class(cmdListClass)
        {
        }
    };

    class VKDevice;

    class IVKObjectDeleter
    {
    public:
        uint32_t FramesLeft = 3;

        virtual ~IVKObjectDeleter() = default;

        virtual void Delete(VKDevice* device) = 0;
    };

#define FE_VK_OBJECT_DELETER(obj)                                                                                                \
    class Osmium_Vulkan_##obj##_Deleter final : public IVKObjectDeleter                                                          \
    {                                                                                                                            \
        Vk##obj m_##obj;                                                                                                         \
                                                                                                                                 \
    public:                                                                                                                      \
        inline explicit Osmium_Vulkan_##obj##_Deleter(Vk##obj handle)                                                            \
            : m_##obj(handle)                                                                                                    \
        {                                                                                                                        \
        }                                                                                                                        \
                                                                                                                                 \
        void Delete(VKDevice* device) override;                                                                                  \
    };                                                                                                                           \
                                                                                                                                 \
    void Osmium_Vulkan_##obj##_Deleter::Delete(VKDevice* device)                                                                 \
    {                                                                                                                            \
        vkDestroy##obj(device->GetNativeDevice(), m_##obj, VK_NULL_HANDLE);                                                      \
    }

#define FE_DELETE_VK_OBJECT(obj, name) m_Device->QueueObjectDelete<Osmium_Vulkan_##obj##_Deleter>(name);

    class DescriptorSetLayoutData final
    {
        VkDescriptorSetLayout m_SetLayout;
        uint32_t m_RefCount;

    public:
        FE_RTTI_Base(DescriptorSetLayoutData, "93EA161C-0BC6-45F6-962B-8A21259DFE2B");

        inline DescriptorSetLayoutData() = default;

        inline explicit DescriptorSetLayoutData(VkDescriptorSetLayout layout)
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

    class VKDevice final
        : public IDevice
        , public EventBus<FrameEvents>::Handler
    {
        VkDevice m_NativeDevice;
        VkPhysicalDevice m_NativeAdapter;
        VKAdapter* m_Adapter;
        VKInstance* m_Instance;
        eastl::vector<VKQueueFamilyData> m_QueueFamilyIndices;

        eastl::vector<VkSemaphore> m_WaitSemaphores;
        eastl::vector<VkSemaphore> m_SignalSemaphores;

        festd::unordered_dense_map<size_t, DescriptorSetLayoutData> m_DescriptorSetLayouts;
        LRUCacheMap<size_t, VkMemoryRequirements> m_ImageMemoryRequirementsByDesc;
        VkMemoryRequirements m_ImageMemoryRequirements;
        VkMemoryRequirements m_RenderTargetMemoryRequirements;
        VkMemoryRequirements m_BufferMemoryRequirements;

        eastl::vector<IVKObjectDeleter*> m_PendingDelete;

        void FindQueueFamilies();

    public:
        FE_RTTI_Class(VKDevice, "7AE4B802-75AF-439E-AA48-BC72761B7B72");

        explicit VKDevice(VKAdapter& adapter);
        ~VKDevice() override;
        VkDevice GetNativeDevice();

        uint32_t FindMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties);

        VkDescriptorSetLayout GetDescriptorSetLayout(const ArraySlice<DescriptorDesc>& descriptors, size_t& key);
        void ReleaseDescriptorSetLayout(size_t key);

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
            return m_QueueFamilyIndices.front().CmdPool;
        }

        inline VkCommandPool GetCommandPool(uint32_t queueFamilyIndex)
        {
            for (auto& queue : m_QueueFamilyIndices)
            {
                if (queue.FamilyIndex == queueFamilyIndex)
                {
                    return queue.CmdPool;
                }
            }

            FE_UNREACHABLE("Couldn't find command pool");
            return m_QueueFamilyIndices.front().CmdPool;
        }

        inline uint32_t GetQueueFamilyIndex(CommandQueueClass cmdQueueClass)
        {
            for (auto& queue : m_QueueFamilyIndices)
            {
                if (queue.Class == cmdQueueClass)
                {
                    return queue.FamilyIndex;
                }
            }

            FE_UNREACHABLE("Couldn't find queue family");
            return static_cast<uint32_t>(-1);
        }

        template<class T, class... Args>
        inline T* QueueObjectDelete(Args&&... args)
        {
            auto* deleter = Memory::DefaultNew<T>(std::forward<Args>(args)...);
            m_PendingDelete.push_back(deleter);
            return deleter;
        }

        void OnFrameEnd(const FrameEventArgs& args) override;

        VkMemoryRequirements GetImageMemoryRequirements(const ImageDesc& desc);
        VkMemoryRequirements GetImageMemoryRequirements();
        VkMemoryRequirements GetRenderTargetMemoryRequirements();
        VkMemoryRequirements GetBufferMemoryRequirements();

        VkSemaphore& AddWaitSemaphore();
        VkSemaphore& AddSignalSemaphore();
        uint32_t GetWaitSemaphores(const VkSemaphore** semaphores);
        uint32_t GetSignalSemaphores(const VkSemaphore** semaphores);

        void WaitIdle() override;
        IAdapter& GetAdapter() override;
        IInstance& GetInstance() override;
        Rc<IFence> CreateFence(FenceState state) override;
        Rc<ICommandQueue> GetCommandQueue(CommandQueueClass cmdQueueClass) override;
        Rc<ICommandBuffer> CreateCommandBuffer(CommandQueueClass cmdQueueClass) override;
        Rc<ISwapChain> CreateSwapChain(const SwapChainDesc& desc) override;
        Rc<IBuffer> CreateBuffer(const BufferDesc& desc) override;
        Rc<IShaderModule> CreateShaderModule(const ShaderModuleDesc& desc) override;
        Rc<IRenderPass> CreateRenderPass(const RenderPassDesc& desc) override;
        Rc<IDescriptorHeap> CreateDescriptorHeap(const DescriptorHeapDesc& desc) override;
        Rc<VKCommandBuffer> CreateCommandBuffer(uint32_t queueFamilyIndex);
        Rc<IShaderCompiler> CreateShaderCompiler() override;
        Rc<IGraphicsPipeline> CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) override;
        Rc<IImageView> CreateImageView(const ImageViewDesc& desc) override;
        Rc<IFramebuffer> CreateFramebuffer(const FramebufferDesc& desc) override;
        Rc<IWindow> CreateWindow(const WindowDesc& desc) override;
        Rc<IImage> CreateImage(const ImageDesc& desc) override;
        Rc<ISampler> CreateSampler(const SamplerDesc& desc) override;
        Rc<ITransientResourceHeap> CreateTransientResourceHeap(const TransientResourceHeapDesc& desc) override;
    };
} // namespace FE::Osmium
