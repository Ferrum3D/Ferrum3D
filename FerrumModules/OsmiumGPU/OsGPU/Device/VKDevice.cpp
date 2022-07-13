#include <FeCore/Console/FeLog.h>
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

        UInt32 familyCount;
        vkGetPhysicalDeviceQueueFamilyProperties(m_NativeAdapter, &familyCount, nullptr);
        List<VkQueueFamilyProperties> families(familyCount, VkQueueFamilyProperties{});
        vkGetPhysicalDeviceQueueFamilyProperties(m_NativeAdapter, &familyCount, families.Data());
        for (USize i = 0; i < families.Size(); ++i)
        {
            UInt32 graphics = VK_QUEUE_TRANSFER_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT;
            UInt32 compute  = VK_QUEUE_TRANSFER_BIT | VK_QUEUE_COMPUTE_BIT;
            UInt32 copy     = VK_QUEUE_TRANSFER_BIT;

            auto idx = static_cast<UInt32>(i);
            if ((families[i].queueFlags & graphics) == graphics && !hasQueueFamily(CommandQueueClass::Graphics))
            {
                m_QueueFamilyIndices.Push(VKQueueFamilyData(idx, families[i].queueCount, CommandQueueClass::Graphics));
            }
            else if ((families[i].queueFlags & compute) == compute && !hasQueueFamily(CommandQueueClass::Compute))
            {
                m_QueueFamilyIndices.Push(VKQueueFamilyData(idx, families[i].queueCount, CommandQueueClass::Compute));
            }
            else if ((families[i].queueFlags & copy) == copy && !hasQueueFamily(CommandQueueClass::Transfer))
            {
                m_QueueFamilyIndices.Push(VKQueueFamilyData(idx, families[i].queueCount, CommandQueueClass::Transfer));
            }
        }
    }

    UInt32 VKDevice::FindMemoryType(UInt32 typeBits, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(m_NativeAdapter, &memProperties);

        for (UInt32 i = 0; i < memProperties.memoryTypeCount; ++i)
        {
            if ((typeBits & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }

        FE_UNREACHABLE("Memory type not found");
        return static_cast<UInt32>(-1);
    }

    VKDevice::VKDevice(VKAdapter& adapter)
        : m_Adapter(&adapter)
        , m_NativeAdapter(adapter.GetNativeAdapter())
        , m_Instance(fe_assert_cast<VKInstance*>(&adapter.GetInstance()))
    {
        FE_LOG_MESSAGE("Creating Vulkan Device on GPU: {}...", StringSlice(m_Adapter->GetDesc().Name));
        FindQueueFamilies();

        UInt32 availableExtCount;
        vkEnumerateDeviceExtensionProperties(m_NativeAdapter, nullptr, &availableExtCount, nullptr);
        List<VkExtensionProperties> availableExt(availableExtCount, VkExtensionProperties{});
        vkEnumerateDeviceExtensionProperties(m_NativeAdapter, nullptr, &availableExtCount, availableExt.Data());
        for (auto& ext : RequiredDeviceExtensions)
        {
            bool found = std::any_of(availableExt.begin(), availableExt.end(), [&](const VkExtensionProperties& props) {
                return StringSlice(ext) == props.extensionName;
            });
            FE_ASSERT_MSG(found, "Vulkan device extension {} was not found", String(ext));
        }

        constexpr Float32 queuePriority = 1.0f;
        List<VkDeviceQueueCreateInfo> queuesCI{};
        for (auto& queue : m_QueueFamilyIndices)
        {
            auto& queueCI            = queuesCI.Emplace();
            queueCI.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCI.queueFamilyIndex = queue.FamilyIndex;
            queueCI.queueCount       = 1;
            queueCI.pQueuePriorities = &queuePriority;
        }

        VkPhysicalDeviceFeatures deviceFeatures{};
        deviceFeatures.geometryShader     = true;
        deviceFeatures.tessellationShader = true;
        deviceFeatures.samplerAnisotropy  = true;
        deviceFeatures.sampleRateShading  = true;

        VkDeviceCreateInfo deviceCI{};
        deviceCI.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCI.queueCreateInfoCount    = static_cast<UInt32>(queuesCI.Size());
        deviceCI.pQueueCreateInfos       = queuesCI.Data();
        deviceCI.pEnabledFeatures        = &deviceFeatures;
        deviceCI.enabledExtensionCount   = static_cast<UInt32>(RequiredDeviceExtensions.size());
        deviceCI.ppEnabledExtensionNames = RequiredDeviceExtensions.data();

        vkCreateDevice(m_NativeAdapter, &deviceCI, VK_NULL_HANDLE, &m_NativeDevice);
        volkLoadDevice(m_NativeDevice);

        for (auto& queue : m_QueueFamilyIndices)
        {
            VkCommandPoolCreateInfo poolCI{};
            poolCI.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            poolCI.queueFamilyIndex = queue.FamilyIndex;
            vkCreateCommandPool(m_NativeDevice, &poolCI, VK_NULL_HANDLE, &queue.CmdPool);
        }

        m_ImageMemoryRequirementsByDesc.SetCapacity(1024);
    }

    VkDevice VKDevice::GetNativeDevice()
    {
        return m_NativeDevice;
    }

    Shared<IFence> VKDevice::CreateFence(FenceState state)
    {
        return MakeShared<VKFence>(*this, state);
    }

    Shared<ICommandQueue> VKDevice::GetCommandQueue(CommandQueueClass cmdQueueClass)
    {
        VKCommandQueueDesc desc{};
        desc.QueueFamilyIndex = GetQueueFamilyIndex(cmdQueueClass);
        desc.QueueIndex       = 0;
        return MakeShared<VKCommandQueue>(*this, desc);
    }

    Shared<ICommandBuffer> VKDevice::CreateCommandBuffer(CommandQueueClass cmdQueueClass)
    {
        return MakeShared<VKCommandBuffer>(*this, cmdQueueClass);
    }

    Shared<ISwapChain> VKDevice::CreateSwapChain(const SwapChainDesc& desc)
    {
        return MakeShared<VKSwapChain>(*this, desc);
    }

    Shared<IBuffer> VKDevice::CreateBuffer(const BufferDesc& desc)
    {
        auto buffer = MakeShared<VKBuffer>(*this, desc);

        VkBufferCreateInfo bufferCI{};
        bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCI.size  = desc.Size;
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

    Shared<IShaderModule> VKDevice::CreateShaderModule(const ShaderModuleDesc& desc)
    {
        return MakeShared<VKShaderModule>(*this, desc);
    }

    Shared<VKCommandBuffer> VKDevice::CreateCommandBuffer(UInt32 queueFamilyIndex)
    {
        return MakeShared<VKCommandBuffer>(*this, queueFamilyIndex);
    }

    Shared<IRenderPass> VKDevice::CreateRenderPass(const RenderPassDesc& desc)
    {
        return MakeShared<VKRenderPass>(*this, desc);
    }

    Shared<IDescriptorHeap> VKDevice::CreateDescriptorHeap(const DescriptorHeapDesc& desc)
    {
        return MakeShared<VKDescriptorHeap>(*this, desc);
    }

    Shared<IShaderCompiler> VKDevice::CreateShaderCompiler()
    {
        return MakeShared<ShaderCompilerDXC>(GraphicsAPI::Vulkan);
    }

    Shared<IGraphicsPipeline> VKDevice::CreateGraphicsPipeline(const GraphicsPipelineDesc& desc)
    {
        return MakeShared<VKGraphicsPipeline>(*this, desc);
    }

    Shared<IImageView> VKDevice::CreateImageView(const ImageViewDesc& desc)
    {
        return MakeShared<VKImageView>(*this, desc);
    }

    Shared<IFramebuffer> VKDevice::CreateFramebuffer(const FramebufferDesc& desc)
    {
        return MakeShared<VKFramebuffer>(*this, desc);
    }

    void VKDevice::WaitIdle()
    {
        vkDeviceWaitIdle(m_NativeDevice);
    }

    VkSemaphore& VKDevice::AddWaitSemaphore()
    {
        return m_WaitSemaphores.Emplace();
    }

    VkSemaphore& VKDevice::AddSignalSemaphore()
    {
        return m_SignalSemaphores.Emplace();
    }

    UInt32 VKDevice::GetWaitSemaphores(const VkSemaphore** semaphores)
    {
        *semaphores = m_WaitSemaphores.Data();
        return static_cast<UInt32>(m_WaitSemaphores.Size());
    }

    UInt32 VKDevice::GetSignalSemaphores(const VkSemaphore** semaphores)
    {
        *semaphores = m_SignalSemaphores.Data();
        return static_cast<UInt32>(m_SignalSemaphores.Size());
    }

    Shared<IWindow> VKDevice::CreateWindow(const WindowDesc& desc)
    {
        return static_pointer_cast<IWindow>(MakeShared<Window>(desc));
    }

    Shared<IImage> VKDevice::CreateImage(const ImageDesc& desc)
    {
        return MakeShared<VKImage>(*this, desc);
    }

    Shared<ISampler> VKDevice::CreateSampler(const SamplerDesc& desc)
    {
        return MakeShared<VKSampler>(*this, desc);
    }

    VKDevice::~VKDevice()
    {
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
            auto bindFlags                   = ImageBindFlags::Color | ImageBindFlags::ShaderRead;
            auto image                       = CreateImage(ImageDesc::Img2D(bindFlags, 1, 1, Format::R8G8B8A8_UNorm));
            auto* vkImage                    = fe_assert_cast<VKImage*>(image.GetRaw());
            m_RenderTargetMemoryRequirements = vkImage->MemoryRequirements;
        }

        return m_RenderTargetMemoryRequirements;
    }

    VkMemoryRequirements VKDevice::GetBufferMemoryRequirements()
    {
        if (m_BufferMemoryRequirements.size == 0)
        {
            auto bindFlags             = BindFlags::ConstantBuffer | BindFlags::ShaderResource;
            auto buffer                = CreateBuffer(BufferDesc(1, bindFlags));
            auto* vkBuffer             = fe_assert_cast<VKBuffer*>(buffer.GetRaw());
            m_BufferMemoryRequirements = vkBuffer->MemoryRequirements;
        }

        return m_BufferMemoryRequirements;
    }

    VkMemoryRequirements VKDevice::GetImageMemoryRequirements(const ImageDesc& desc)
    {
        VkMemoryRequirements result{};
        USize hash = 0;
        HashCombine(hash, desc);
        if (m_ImageMemoryRequirementsByDesc.TryGetValue(hash, result))
        {
            return result;
        }

        auto image    = CreateImage(desc);
        auto* vkImage = fe_assert_cast<VKImage*>(image.GetRaw());
        m_ImageMemoryRequirementsByDesc.Emplace(hash, vkImage->MemoryRequirements);
        return vkImage->MemoryRequirements;
    }

    VkMemoryRequirements VKDevice::GetImageMemoryRequirements()
    {
        if (m_ImageMemoryRequirements.size == 0)
        {
            auto bindFlags            = ImageBindFlags::UnorderedAccess | ImageBindFlags::ShaderRead;
            auto image                = CreateImage(ImageDesc::Img2D(bindFlags, 1, 1, Format::R8G8B8A8_UNorm));
            auto* vkImage             = fe_assert_cast<VKImage*>(image.GetRaw());
            m_ImageMemoryRequirements = vkImage->MemoryRequirements;
        }

        return m_ImageMemoryRequirements;
    }

    Shared<ITransientResourceHeap> VKDevice::CreateTransientResourceHeap(const TransientResourceHeapDesc& desc)
    {
        return MakeShared<VKTransientResourceHeap>(*this, desc);
    }
} // namespace FE::Osmium
