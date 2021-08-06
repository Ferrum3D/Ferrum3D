#include <FeCore/Console/FeLog.h>
#include <FeGPU/Adapter/VKAdapter.h>
#include <FeGPU/CommandBuffer/VKCommandBuffer.h>
#include <FeGPU/CommandQueue/VKCommandQueue.h>
#include <FeGPU/Device/VKDevice.h>
#include <FeGPU/Fence/VKFence.h>
#include <algorithm>
#include <FeGPU/Swapchain/VKSwapChain.h>
#include <FeGPU/Buffer/VKBuffer.h>

namespace FE::GPU
{
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
                return i;
        }

        FE_UNREACHABLE("Memory type not found");
        return -1;
    }

    VKDevice::VKDevice(VKAdapter& adapter)
        : m_Adapter(&adapter)
        , m_NativeAdapter(&adapter.GetNativeAdapter())
        , m_Instance(static_cast<VKInstance*>(&adapter.GetInstance()))
    {
        FindQueueFamilies();

        auto availableExt = m_NativeAdapter->enumerateDeviceExtensionProperties<StdHeapAllocator<vk::ExtensionProperties>>();
        for (auto& ext : RequiredDeviceExtensions)
        {
            bool found = std::any_of(availableExt.begin(), availableExt.end(), [&](const vk::ExtensionProperties& props) {
                return StringSlice(ext) == props.extensionName.data();
            });
            FE_ASSERT_MSG(found, "Vulkan device extension {} was not found", ext);
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

        vk::PhysicalDeviceVulkan12Features deviceFeatures12{};
        deviceFeatures12.timelineSemaphore = true;

        vk::DeviceCreateInfo deviceCI{};
        deviceCI.queueCreateInfoCount    = static_cast<UInt32>(queuesCI.size());
        deviceCI.pQueueCreateInfos       = queuesCI.data();
        deviceCI.pEnabledFeatures        = &deviceFeatures;
        deviceCI.enabledExtensionCount   = static_cast<UInt32>(RequiredDeviceExtensions.size());
        deviceCI.ppEnabledExtensionNames = RequiredDeviceExtensions.data();
        deviceCI.pNext                   = &deviceFeatures12;

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

    RefCountPtr<IFence> VKDevice::CreateFence(UInt64 value)
    {
        return StaticPtrCast<IFence>(MakeShared<VKFence>(*this, value));
    }

    RefCountPtr<ICommandQueue> VKDevice::GetCommandQueue(CommandQueueClass cmdQueueClass)
    {
        VKCommandQueueDesc desc{};
        desc.QueueFamilyIndex = GetQueueFamilyIndex(cmdQueueClass);
        desc.QueueIndex       = 0;
        return StaticPtrCast<ICommandQueue>(MakeShared<VKCommandQueue>(*this, desc));
    }

    RefCountPtr<ICommandBuffer> VKDevice::CreateCommandBuffer(CommandQueueClass cmdQueueClass)
    {
        return StaticPtrCast<ICommandBuffer>(MakeShared<VKCommandBuffer>(*this, cmdQueueClass));
    }


    RefCountPtr<ISwapChain> VKDevice::CreateSwapChain(const SwapChainDesc& desc)
    {
        return StaticPtrCast<ISwapChain>(MakeShared<VKSwapChain>(*this, desc));
    }

    RefCountPtr<IBuffer> VKDevice::CreateBuffer(BindFlags bindFlags, UInt64 size)
    {
        BufferDesc desc{};
        desc.Size = size;
        auto buffer = MakeShared<VKBuffer>(*this, desc);

        vk::BufferCreateInfo bufferCI = {};
        bufferCI.size = size;
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
            bufferCI.usage |= vk::BufferUsageFlagBits::eVertexBuffer;
        if ((bindFlags & BindFlags::IndexBuffer) != BindFlags::None)
            bufferCI.usage |= vk::BufferUsageFlagBits::eIndexBuffer;
        if ((bindFlags & BindFlags::ConstantBuffer) != BindFlags::None)
            bufferCI.usage |= vk::BufferUsageFlagBits::eUniformBuffer;
        if ((bindFlags & BindFlags::IndirectDrawArgs) != BindFlags::None)
            bufferCI.usage |= vk::BufferUsageFlagBits::eIndirectBuffer;

        buffer->Buffer = m_NativeDevice->createBufferUnique(bufferCI);

        return StaticPtrCast<IBuffer>(buffer);
    }

    IInstance& VKDevice::GetInstance()
    {
        return *m_Instance;
    }

    IAdapter& VKDevice::GetAdapter()
    {
        return *m_Adapter;
    }
} // namespace FE::GPU
