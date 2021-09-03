#include <FeCore/Console/FeLog.h>
#include <FeGPU/Adapter/VKAdapter.h>
#include <FeGPU/Buffer/VKBuffer.h>
#include <FeGPU/CommandBuffer/VKCommandBuffer.h>
#include <FeGPU/CommandQueue/VKCommandQueue.h>
#include <FeGPU/Descriptors/VKDescriptorHeap.h>
#include <FeGPU/Device/VKDevice.h>
#include <FeGPU/Fence/VKFence.h>
#include <FeGPU/Framebuffer/VKFramebuffer.h>
#include <FeGPU/ImageView/VKImageView.h>
#include <FeGPU/Pipeline/VKGraphicsPipeline.h>
#include <FeGPU/RenderPass/VKRenderPass.h>
#include <FeGPU/Shader/ShaderCompilerDXC.h>
#include <FeGPU/Shader/VKShaderModule.h>
#include <FeGPU/SwapChain/VKSwapChain.h>
#include <FeGPU/Window/Window.h>
#include <algorithm>

namespace FE::GPU
{
    constexpr auto RequiredDeviceExtensions = std::array{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    void VKDevice::FindQueueFamilies()
    {
        auto hasQueueFamily = [this](CommandQueueClass cmdQueueClass) {
            return std::any_of(m_QueueFamilyIndices.begin(), m_QueueFamilyIndices.end(), [=](const VKQueueFamilyData& data) {
                return data.Class == cmdQueueClass;
            });
        };

        auto families = m_NativeAdapter->getQueueFamilyProperties<StdHeapAllocator<vk::QueueFamilyProperties>>();
        for (size_t i = 0; i < families.size(); ++i)
        {
            auto graphics = vk::QueueFlagBits::eTransfer | vk::QueueFlagBits::eCompute | vk::QueueFlagBits::eGraphics;
            auto compute  = vk::QueueFlagBits::eTransfer | vk::QueueFlagBits::eCompute;
            auto copy     = vk::QueueFlagBits::eTransfer;
            auto idx      = static_cast<UInt32>(i);

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

    UInt32 VKDevice::FindMemoryType(UInt32 typeBits, vk::MemoryPropertyFlags properties)
    {
        vk::PhysicalDeviceMemoryProperties memProperties;
        m_Adapter->GetNativeAdapter().getMemoryProperties(&memProperties);

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
        , m_NativeAdapter(&adapter.GetNativeAdapter())
        , m_Instance(fe_dynamic_cast<VKInstance*>(&adapter.GetInstance()))
    {
        FE_LOG_MESSAGE("Creating Vulkan Device on GPU: {}...", m_Adapter->GetDesc().Name);
        FindQueueFamilies();

        auto availableExt = m_NativeAdapter->enumerateDeviceExtensionProperties<StdHeapAllocator<vk::ExtensionProperties>>();
        for (auto& ext : RequiredDeviceExtensions)
        {
            bool found = std::any_of(availableExt.begin(), availableExt.end(), [&](const vk::ExtensionProperties& props) {
                return StringSlice(ext) == props.extensionName.data();
            });
            FE_ASSERT_MSG(found, "Vulkan device extension {} was not found", String(ext));
        }

        constexpr Float32 queuePriority = 1.0f;
        Vector<vk::DeviceQueueCreateInfo> queuesCI{};
        for (auto& queue : m_QueueFamilyIndices)
        {
            vk::DeviceQueueCreateInfo& queueCI = queuesCI.emplace_back();
            queueCI.queueFamilyIndex           = queue.FamilyIndex;
            queueCI.queueCount                 = 1;
            queueCI.pQueuePriorities           = &queuePriority;
        }

        vk::PhysicalDeviceFeatures deviceFeatures{};
        deviceFeatures.geometryShader     = true;
        deviceFeatures.tessellationShader = true;

        vk::DeviceCreateInfo deviceCI{};
        deviceCI.queueCreateInfoCount    = static_cast<UInt32>(queuesCI.size());
        deviceCI.pQueueCreateInfos       = queuesCI.data();
        deviceCI.pEnabledFeatures        = &deviceFeatures;
        deviceCI.enabledExtensionCount   = static_cast<UInt32>(RequiredDeviceExtensions.size());
        deviceCI.ppEnabledExtensionNames = RequiredDeviceExtensions.data();

        m_NativeDevice = m_NativeAdapter->createDeviceUnique(deviceCI);
        VULKAN_HPP_DEFAULT_DISPATCHER.init(m_NativeDevice.get());

        for (auto& queue : m_QueueFamilyIndices)
        {
            vk::CommandPoolCreateInfo poolCI{};
            poolCI.queueFamilyIndex = queue.FamilyIndex;
            queue.CmdPool           = m_NativeDevice->createCommandPoolUnique(poolCI);
        }
    }

    vk::Device& VKDevice::GetNativeDevice()
    {
        return m_NativeDevice.get();
    }

    Shared<IFence> VKDevice::CreateFence(FenceState state)
    {
        return static_pointer_cast<IFence>(MakeShared<VKFence>(*this, state));
    }

    Shared<ICommandQueue> VKDevice::GetCommandQueue(CommandQueueClass cmdQueueClass)
    {
        VKCommandQueueDesc desc{};
        desc.QueueFamilyIndex = GetQueueFamilyIndex(cmdQueueClass);
        desc.QueueIndex       = 0;
        return static_pointer_cast<ICommandQueue>(MakeShared<VKCommandQueue>(*this, desc));
    }

    Shared<ICommandBuffer> VKDevice::CreateCommandBuffer(CommandQueueClass cmdQueueClass)
    {
        return static_pointer_cast<ICommandBuffer>(MakeShared<VKCommandBuffer>(*this, cmdQueueClass));
    }

    Shared<ISwapChain> VKDevice::CreateSwapChain(const SwapChainDesc& desc)
    {
        return static_pointer_cast<ISwapChain>(MakeShared<VKSwapChain>(*this, desc));
    }

    Shared<IBuffer> VKDevice::CreateBuffer(BindFlags bindFlags, UInt64 size)
    {
        BufferDesc desc{};
        desc.Size   = size;
        auto buffer = MakeShared<VKBuffer>(*this, desc);

        vk::BufferCreateInfo bufferCI{};
        bufferCI.size  = size;
        bufferCI.usage = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc;

        if ((bindFlags & BindFlags::ShaderResource) != BindFlags::None)
        {
            bufferCI.usage |= vk::BufferUsageFlagBits::eUniformTexelBuffer;
            bufferCI.usage |= vk::BufferUsageFlagBits::eStorageBuffer;
        }
        if ((bindFlags & BindFlags::UnorderedAccess) != BindFlags::None)
        {
            bufferCI.usage |= vk::BufferUsageFlagBits::eStorageBuffer;
            bufferCI.usage |= vk::BufferUsageFlagBits::eStorageTexelBuffer;
        }
        if ((bindFlags & BindFlags::VertexBuffer) != BindFlags::None)
        {
            bufferCI.usage |= vk::BufferUsageFlagBits::eVertexBuffer;
        }
        if ((bindFlags & BindFlags::IndexBuffer) != BindFlags::None)
        {
            bufferCI.usage |= vk::BufferUsageFlagBits::eIndexBuffer;
        }
        if ((bindFlags & BindFlags::ConstantBuffer) != BindFlags::None)
        {
            bufferCI.usage |= vk::BufferUsageFlagBits::eUniformBuffer;
        }
        if ((bindFlags & BindFlags::IndirectDrawArgs) != BindFlags::None)
        {
            bufferCI.usage |= vk::BufferUsageFlagBits::eIndirectBuffer;
        }

        buffer->Buffer = m_NativeDevice->createBufferUnique(bufferCI);

        return static_pointer_cast<IBuffer>(buffer);
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
        return static_pointer_cast<IShaderModule>(MakeShared<VKShaderModule>(*this, desc));
    }

    Shared<VKCommandBuffer> VKDevice::CreateCommandBuffer(UInt32 queueFamilyIndex)
    {
        return MakeShared<VKCommandBuffer>(*this, queueFamilyIndex);
    }

    Shared<IRenderPass> VKDevice::CreateRenderPass(const RenderPassDesc& desc)
    {
        return static_pointer_cast<IRenderPass>(MakeShared<VKRenderPass>(*this, desc));
    }

    Shared<IDescriptorHeap> VKDevice::CreateDescriptorHeap(const DescriptorHeapDesc& desc)
    {
        return static_pointer_cast<IDescriptorHeap>(MakeShared<VKDescriptorHeap>(*this, desc));
    }

    Shared<IShaderCompiler> VKDevice::CreateShaderCompiler()
    {
        return static_pointer_cast<IShaderCompiler>(MakeShared<ShaderCompilerDXC>(GraphicsAPI::Vulkan));
    }

    Shared<IGraphicsPipeline> VKDevice::CreateGraphicsPipeline(const GraphicsPipelineDesc& desc)
    {
        return static_pointer_cast<IGraphicsPipeline>(MakeShared<VKGraphicsPipeline>(*this, desc));
    }

    Shared<IImageView> VKDevice::CreateImageView(const ImageViewDesc& desc)
    {
        return static_pointer_cast<IImageView>(MakeShared<VKImageView>(*this, desc));
    }

    Shared<IFramebuffer> VKDevice::CreateFramebuffer(const FramebufferDesc& desc)
    {
        return static_pointer_cast<IFramebuffer>(MakeShared<VKFramebuffer>(*this, desc));
    }

    void VKDevice::WaitIdle()
    {
        m_NativeDevice->waitIdle();
    }

    vk::Semaphore& VKDevice::AddWaitSemaphore()
    {
        return m_WaitSemaphores.emplace_back();
    }

    vk::Semaphore& VKDevice::AddSignalSemaphore()
    {
        return m_SignalSemaphores.emplace_back();
    }

    UInt32 VKDevice::GetWaitSemaphores(const vk::Semaphore** semaphores)
    {
        *semaphores = m_WaitSemaphores.data();
        return static_cast<UInt32>(m_WaitSemaphores.size());
    }

    UInt32 VKDevice::GetSignalSemaphores(const vk::Semaphore** semaphores)
    {
        *semaphores = m_SignalSemaphores.data();
        return static_cast<UInt32>(m_SignalSemaphores.size());
    }

    Shared<IWindow> VKDevice::CreateWindow(const WindowDesc& desc)
    {
        return static_pointer_cast<IWindow>(MakeShared<Window>(desc));
    }
} // namespace FE::GPU
