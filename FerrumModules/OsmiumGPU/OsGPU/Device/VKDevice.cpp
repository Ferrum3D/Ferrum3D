#include <FeCore/Console/FeLog.h>
#include <FeCore/Containers/ArraySlice.h>
#include <OsGPU/Adapter/VKAdapter.h>
#include <OsGPU/Buffer/VKBuffer.h>
#include <OsGPU/CommandBuffer/VKCommandBuffer.h>
#include <OsGPU/CommandQueue/VKCommandQueue.h>
#include <OsGPU/Descriptors/VKDescriptorHeap.h>
#include <OsGPU/Device/VKDevice.h>
#include <OsGPU/Fence/VKFence.h>
#include <OsGPU/Framebuffer/VKFramebuffer.h>
#include <OsGPU/Image/VKImage.h>
#include <OsGPU/ImageView/VKImageView.h>
#include <OsGPU/Instance/VKInstance.h>
#include <OsGPU/Pipeline/VKGraphicsPipeline.h>
#include <OsGPU/RenderPass/VKRenderPass.h>
#include <OsGPU/Resource/VKTransientResourceHeap.h>
#include <OsGPU/Sampler/VKSampler.h>
#include <OsGPU/Shader/ShaderCompilerDXC.h>
#include <OsGPU/Shader/VKShaderModule.h>
#include <OsGPU/SwapChain/VKSwapChain.h>
#include <OsGPU/Window/Window.h>
#include <algorithm>

namespace FE::Osmium
{
    constexpr auto RequiredDeviceExtensions = std::array{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    void VKDevice::FindQueueFamilies()
    {
        auto hasQueueFamily = [this](CommandQueueClass cmdQueueClass) {
            return std::any_of(m_QueueFamilyIndices.begin(), m_QueueFamilyIndices.end(), [=](const VKQueueFamilyData& data) {
                return data.Class == cmdQueueClass;
            });
        };

        uint32_t familyCount;
        vkGetPhysicalDeviceQueueFamilyProperties(m_NativeAdapter, &familyCount, nullptr);
        eastl::vector<VkQueueFamilyProperties> families(familyCount, VkQueueFamilyProperties{});
        vkGetPhysicalDeviceQueueFamilyProperties(m_NativeAdapter, &familyCount, families.data());
        for (uint32_t i = 0; i < families.size(); ++i)
        {
            uint32_t graphics = VK_QUEUE_TRANSFER_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT;
            uint32_t compute = VK_QUEUE_TRANSFER_BIT | VK_QUEUE_COMPUTE_BIT;
            uint32_t copy = VK_QUEUE_TRANSFER_BIT;

            auto idx = static_cast<uint32_t>(i);
            if ((families[i].queueFlags & graphics) == graphics && !hasQueueFamily(CommandQueueClass::Graphics))
            {
                m_QueueFamilyIndices.push_back(VKQueueFamilyData(idx, families[i].queueCount, CommandQueueClass::Graphics));
            }
            else if ((families[i].queueFlags & compute) == compute && !hasQueueFamily(CommandQueueClass::Compute))
            {
                m_QueueFamilyIndices.push_back(VKQueueFamilyData(idx, families[i].queueCount, CommandQueueClass::Compute));
            }
            else if ((families[i].queueFlags & copy) == copy && !hasQueueFamily(CommandQueueClass::Transfer))
            {
                m_QueueFamilyIndices.push_back(VKQueueFamilyData(idx, families[i].queueCount, CommandQueueClass::Transfer));
            }
        }
    }

    uint32_t VKDevice::FindMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(m_NativeAdapter, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
        {
            if ((typeBits & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }

        FE_UNREACHABLE("Memory type not found");
        return static_cast<uint32_t>(-1);
    }

    VKDevice::VKDevice(VKAdapter& adapter)
        : m_Adapter(&adapter)
        , m_NativeAdapter(adapter.GetNativeAdapter())
        , m_Instance(fe_assert_cast<VKInstance*>(&adapter.GetInstance()))
    {
        FE_LOG_MESSAGE("Creating Vulkan Device on GPU: {}...", StringSlice(m_Adapter->GetDesc().Name));
        FindQueueFamilies();

        uint32_t availableExtCount;
        vkEnumerateDeviceExtensionProperties(m_NativeAdapter, nullptr, &availableExtCount, nullptr);
        eastl::vector<VkExtensionProperties> availableExt(availableExtCount, VkExtensionProperties{});
        vkEnumerateDeviceExtensionProperties(m_NativeAdapter, nullptr, &availableExtCount, availableExt.data());
        for (auto& ext : RequiredDeviceExtensions)
        {
            bool found = std::any_of(availableExt.begin(), availableExt.end(), [&](const VkExtensionProperties& props) {
                return StringSlice(ext) == props.extensionName;
            });
            FE_ASSERT_MSG(found, "Vulkan device extension {} was not found", String(ext));
        }

        constexpr float queuePriority = 1.0f;
        eastl::vector<VkDeviceQueueCreateInfo> queuesCI{};
        for (auto& queue : m_QueueFamilyIndices)
        {
            auto& queueCI = queuesCI.push_back();
            queueCI.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCI.queueFamilyIndex = queue.FamilyIndex;
            queueCI.queueCount = 1;
            queueCI.pQueuePriorities = &queuePriority;
        }

        VkPhysicalDeviceFeatures deviceFeatures{};
        deviceFeatures.geometryShader = true;
        deviceFeatures.tessellationShader = true;
        deviceFeatures.samplerAnisotropy = true;
        deviceFeatures.sampleRateShading = true;

        VkDeviceCreateInfo deviceCI{};
        deviceCI.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCI.queueCreateInfoCount = static_cast<uint32_t>(queuesCI.size());
        deviceCI.pQueueCreateInfos = queuesCI.data();
        deviceCI.pEnabledFeatures = &deviceFeatures;
        deviceCI.enabledExtensionCount = static_cast<uint32_t>(RequiredDeviceExtensions.size());
        deviceCI.ppEnabledExtensionNames = RequiredDeviceExtensions.data();

        vkCreateDevice(m_NativeAdapter, &deviceCI, VK_NULL_HANDLE, &m_NativeDevice);
        volkLoadDevice(m_NativeDevice);

        for (auto& queue : m_QueueFamilyIndices)
        {
            VkCommandPoolCreateInfo poolCI{};
            poolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            poolCI.queueFamilyIndex = queue.FamilyIndex;
            vkCreateCommandPool(m_NativeDevice, &poolCI, VK_NULL_HANDLE, &queue.CmdPool);
        }

        m_ImageMemoryRequirementsByDesc.SetCapacity(1024);
    }

    VkDevice VKDevice::GetNativeDevice()
    {
        return m_NativeDevice;
    }

    Rc<IFence> VKDevice::CreateFence(FenceState state)
    {
        return Rc<VKFence>::DefaultNew(*this, state);
    }

    Rc<ICommandQueue> VKDevice::GetCommandQueue(CommandQueueClass cmdQueueClass)
    {
        VKCommandQueueDesc desc{};
        desc.QueueFamilyIndex = GetQueueFamilyIndex(cmdQueueClass);
        desc.QueueIndex = 0;
        return Rc<VKCommandQueue>::DefaultNew(*this, desc);
    }

    Rc<ICommandBuffer> VKDevice::CreateCommandBuffer(CommandQueueClass cmdQueueClass)
    {
        return Rc<VKCommandBuffer>::DefaultNew(*this, cmdQueueClass);
    }

    Rc<ISwapChain> VKDevice::CreateSwapChain(const SwapChainDesc& desc)
    {
        return Rc<VKSwapChain>::DefaultNew(*this, desc);
    }

    Rc<IBuffer> VKDevice::CreateBuffer(const BufferDesc& desc)
    {
        Rc buffer = Rc<VKBuffer>::DefaultNew(*this, desc);

        VkBufferCreateInfo bufferCI{};
        bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCI.size = desc.Size;
        bufferCI.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        if ((desc.Flags & BindFlags::ShaderResource) != BindFlags::None)
        {
            bufferCI.usage |= VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
            bufferCI.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        }
        if ((desc.Flags & BindFlags::UnorderedAccess) != BindFlags::None)
        {
            bufferCI.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
            bufferCI.usage |= VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
        }
        if ((desc.Flags & BindFlags::VertexBuffer) != BindFlags::None)
        {
            bufferCI.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        }
        if ((desc.Flags & BindFlags::IndexBuffer) != BindFlags::None)
        {
            bufferCI.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        }
        if ((desc.Flags & BindFlags::ConstantBuffer) != BindFlags::None)
        {
            bufferCI.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        }
        if ((desc.Flags & BindFlags::IndirectDrawArgs) != BindFlags::None)
        {
            bufferCI.usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
        }

        vkCreateBuffer(m_NativeDevice, &bufferCI, VK_NULL_HANDLE, &buffer->Buffer);
        vkGetBufferMemoryRequirements(m_NativeDevice, buffer->Buffer, &buffer->MemoryRequirements);
        return buffer;
    }

    IInstance& VKDevice::GetInstance()
    {
        return *m_Instance;
    }

    IAdapter& VKDevice::GetAdapter()
    {
        return *m_Adapter;
    }

    Rc<IShaderModule> VKDevice::CreateShaderModule(const ShaderModuleDesc& desc)
    {
        return Rc<VKShaderModule>::DefaultNew(*this, desc);
    }

    Rc<VKCommandBuffer> VKDevice::CreateCommandBuffer(uint32_t queueFamilyIndex)
    {
        return Rc<VKCommandBuffer>::DefaultNew(*this, queueFamilyIndex);
    }

    Rc<IRenderPass> VKDevice::CreateRenderPass(const RenderPassDesc& desc)
    {
        return Rc<VKRenderPass>::DefaultNew(*this, desc);
    }

    Rc<IDescriptorHeap> VKDevice::CreateDescriptorHeap(const DescriptorHeapDesc& desc)
    {
        return Rc<VKDescriptorHeap>::DefaultNew(*this, desc);
    }

    Rc<IShaderCompiler> VKDevice::CreateShaderCompiler()
    {
        return Rc<ShaderCompilerDXC>::DefaultNew(GraphicsAPI::Vulkan);
    }

    Rc<IGraphicsPipeline> VKDevice::CreateGraphicsPipeline(const GraphicsPipelineDesc& desc)
    {
        return Rc<VKGraphicsPipeline>::DefaultNew(*this, desc);
    }

    Rc<IImageView> VKDevice::CreateImageView(const ImageViewDesc& desc)
    {
        return Rc<VKImageView>::DefaultNew(*this, desc);
    }

    Rc<IFramebuffer> VKDevice::CreateFramebuffer(const FramebufferDesc& desc)
    {
        return Rc<VKFramebuffer>::DefaultNew(*this, desc);
    }

    void VKDevice::WaitIdle()
    {
        vkDeviceWaitIdle(m_NativeDevice);
    }

    VkSemaphore& VKDevice::AddWaitSemaphore()
    {
        return m_WaitSemaphores.push_back();
    }

    VkSemaphore& VKDevice::AddSignalSemaphore()
    {
        return m_SignalSemaphores.push_back();
    }

    uint32_t VKDevice::GetWaitSemaphores(const VkSemaphore** semaphores)
    {
        *semaphores = m_WaitSemaphores.data();
        return static_cast<uint32_t>(m_WaitSemaphores.size());
    }

    uint32_t VKDevice::GetSignalSemaphores(const VkSemaphore** semaphores)
    {
        *semaphores = m_SignalSemaphores.data();
        return static_cast<uint32_t>(m_SignalSemaphores.size());
    }

    Rc<IWindow> VKDevice::CreateWindow(const WindowDesc& desc)
    {
        return Rc<Window>::DefaultNew(desc);
    }

    Rc<IImage> VKDevice::CreateImage(const ImageDesc& desc)
    {
        return Rc<VKImage>::DefaultNew(*this, desc);
    }

    Rc<ISampler> VKDevice::CreateSampler(const SamplerDesc& desc)
    {
        return Rc<VKSampler>::DefaultNew(*this, desc);
    }

    VKDevice::~VKDevice()
    {
        for (auto& deleter : m_PendingDelete)
        {
            deleter->Delete(this);
            Memory::DefaultDelete(deleter);
            FE_LOG_MESSAGE("Deleted object at {}", deleter);
        }

        for (auto& family : m_QueueFamilyIndices)
        {
            vkDestroyCommandPool(m_NativeDevice, family.CmdPool, VK_NULL_HANDLE);
        }

        vkDestroyDevice(m_NativeDevice, VK_NULL_HANDLE);
    }

    VkMemoryRequirements VKDevice::GetRenderTargetMemoryRequirements()
    {
        if (m_RenderTargetMemoryRequirements.size == 0)
        {
            auto bindFlags = ImageBindFlags::Color | ImageBindFlags::ShaderRead;
            auto image = CreateImage(ImageDesc::Img2D(bindFlags, 1, 1, Format::R8G8B8A8_UNorm));
            auto* vkImage = fe_assert_cast<VKImage*>(image.Get());
            m_RenderTargetMemoryRequirements = vkImage->MemoryRequirements;
        }

        return m_RenderTargetMemoryRequirements;
    }

    VkMemoryRequirements VKDevice::GetBufferMemoryRequirements()
    {
        if (m_BufferMemoryRequirements.size == 0)
        {
            auto bindFlags = BindFlags::ConstantBuffer | BindFlags::ShaderResource;
            auto buffer = CreateBuffer(BufferDesc(1, bindFlags));
            auto* vkBuffer = fe_assert_cast<VKBuffer*>(buffer.Get());
            m_BufferMemoryRequirements = vkBuffer->MemoryRequirements;
        }

        return m_BufferMemoryRequirements;
    }

    VkMemoryRequirements VKDevice::GetImageMemoryRequirements(const ImageDesc& desc)
    {
        VkMemoryRequirements result{};
        size_t hash = 0;
        HashCombine(hash, desc);
        if (m_ImageMemoryRequirementsByDesc.TryGetValue(hash, result))
        {
            return result;
        }

        auto image = CreateImage(desc);
        auto* vkImage = fe_assert_cast<VKImage*>(image.Get());
        m_ImageMemoryRequirementsByDesc.Emplace(hash, vkImage->MemoryRequirements);
        return vkImage->MemoryRequirements;
    }

    VkMemoryRequirements VKDevice::GetImageMemoryRequirements()
    {
        if (m_ImageMemoryRequirements.size == 0)
        {
            auto bindFlags = ImageBindFlags::UnorderedAccess | ImageBindFlags::ShaderRead;
            auto image = CreateImage(ImageDesc::Img2D(bindFlags, 1, 1, Format::R8G8B8A8_UNorm));
            auto* vkImage = fe_assert_cast<VKImage*>(image.Get());
            m_ImageMemoryRequirements = vkImage->MemoryRequirements;
        }

        return m_ImageMemoryRequirements;
    }

    Rc<ITransientResourceHeap> VKDevice::CreateTransientResourceHeap(const TransientResourceHeapDesc& desc)
    {
        return Rc<VKTransientResourceHeap>::DefaultNew(*this, desc);
    }

    inline size_t GetSetLayoutHash(const ArraySlice<DescriptorDesc>& descriptors)
    {
        size_t result = 0;
        for (auto& descriptor : descriptors)
        {
            HashCombine(result, descriptor);
        }

        return result;
    }

    VkDescriptorSetLayout VKDevice::GetDescriptorSetLayout(const ArraySlice<DescriptorDesc>& descriptors, size_t& key)
    {
        key = GetSetLayoutHash(descriptors);
        VkDescriptorSetLayout layout;

        if (m_DescriptorSetLayouts.find(key) == m_DescriptorSetLayouts.end())
        {
            eastl::vector<VkDescriptorSetLayoutBinding> bindings;
            for (uint32_t i = 0; i < descriptors.Length(); ++i)
            {
                auto& desc = descriptors[i];
                auto& binding = bindings.push_back();

                binding.binding = i;
                binding.descriptorCount = desc.Count;
                binding.descriptorType = GetDescriptorType(desc.ResourceType);
                binding.stageFlags = VKConvert(desc.Stage);
            }

            VkDescriptorSetLayoutCreateInfo layoutCI{};
            layoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutCI.bindingCount = static_cast<uint32_t>(bindings.size());
            layoutCI.pBindings = bindings.data();
            vkCreateDescriptorSetLayout(m_NativeDevice, &layoutCI, VK_NULL_HANDLE, &layout);

            m_DescriptorSetLayouts[key] = DescriptorSetLayoutData(layout);
        }
        else
        {
            layout = m_DescriptorSetLayouts[key].SetLayout();
        }

        return layout;
    }

    void VKDevice::ReleaseDescriptorSetLayout(size_t key)
    {
        if (m_DescriptorSetLayouts[key].Release(m_NativeDevice))
        {
            m_DescriptorSetLayouts.erase(key);
        }
    }

    void VKDevice::OnFrameEnd(const FrameEventArgs& /* args */)
    {
        for (uint32_t i = 0; i < m_PendingDelete.size();)
        {
            FE_LOG_MESSAGE(
                "Trying to delete object at {}, frames left: {}...", m_PendingDelete[i], m_PendingDelete[i]->FramesLeft);
            if (--m_PendingDelete[i]->FramesLeft > 0)
            {
                ++i;
                continue;
            }

            m_PendingDelete[i]->Delete(this);
            Memory::DefaultDelete(m_PendingDelete[i]);
            FE_LOG_MESSAGE("Deleted object at {}", m_PendingDelete[i]);
            m_PendingDelete.erase_unsorted(m_PendingDelete.begin() + i);
        }
    }
} // namespace FE::Osmium
