#include <FeCore/Containers/SmallVector.h>
#include <FeCore/Logging/Trace.h>
#include <HAL/DeviceObject.h>
#include <HAL/Image.h>
#include <HAL/ShaderCompilerDXC.h>
#include <HAL/Vulkan/CommandQueue.h>
#include <HAL/Vulkan/Common/BaseTypes.h>
#include <HAL/Vulkan/Device.h>
#include <HAL/Vulkan/DeviceFactory.h>
#include <HAL/Window.h>
#include <algorithm>

namespace FE::Graphics::Vulkan
{
    constexpr auto RequiredDeviceExtensions = std::array{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };


    void Device::FindQueueFamilies()
    {
        auto hasQueueFamily = [this](HAL::HardwareQueueKindFlags cmdQueueClass) {
            return std::any_of(m_QueueFamilyIndices.begin(), m_QueueFamilyIndices.end(), [=](const QueueFamilyData& data) {
                return data.Class == cmdQueueClass;
            });
        };

        uint32_t familyCount;
        vkGetPhysicalDeviceQueueFamilyProperties(m_NativeAdapter, &familyCount, nullptr);

        festd::small_vector<VkQueueFamilyProperties> families(familyCount, VkQueueFamilyProperties{});
        vkGetPhysicalDeviceQueueFamilyProperties(m_NativeAdapter, &familyCount, families.data());
        for (uint32_t i = 0; i < families.size(); ++i)
        {
            uint32_t graphics = VK_QUEUE_TRANSFER_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT;
            uint32_t compute = VK_QUEUE_TRANSFER_BIT | VK_QUEUE_COMPUTE_BIT;
            uint32_t copy = VK_QUEUE_TRANSFER_BIT;

            auto idx = static_cast<uint32_t>(i);
            if ((families[i].queueFlags & graphics) == graphics && !hasQueueFamily(HAL::HardwareQueueKindFlags::kGraphics))
            {
                m_QueueFamilyIndices.push_back(
                    QueueFamilyData(idx, families[i].queueCount, HAL::HardwareQueueKindFlags::kGraphics));
            }
            else if ((families[i].queueFlags & compute) == compute && !hasQueueFamily(HAL::HardwareQueueKindFlags::kCompute))
            {
                m_QueueFamilyIndices.push_back(
                    QueueFamilyData(idx, families[i].queueCount, HAL::HardwareQueueKindFlags::kCompute));
            }
            else if ((families[i].queueFlags & copy) == copy && !hasQueueFamily(HAL::HardwareQueueKindFlags::kTransfer))
            {
                m_QueueFamilyIndices.push_back(
                    QueueFamilyData(idx, families[i].queueCount, HAL::HardwareQueueKindFlags::kTransfer));
            }
        }
    }


    uint32_t Device::FindMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties)
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

        FE_AssertMsg(false, "Memory type not found");
        return static_cast<uint32_t>(-1);
    }


    Device::Device(Logger* pLogger, HAL::DeviceFactory* pFactory)
        : HAL::Device(pLogger)
        , m_pDeviceFactory(ImplCast(pFactory))
    {
    }


    HAL::ResultCode Device::Init(VkPhysicalDevice nativeAdapter)
    {
        m_NativeAdapter = nativeAdapter;

        vkGetPhysicalDeviceProperties(nativeAdapter, &m_AdapterProperties);
        m_pLogger->LogInfo("Creating Vulkan Device on GPU: {}...", m_AdapterProperties.deviceName);

        FindQueueFamilies();

        uint32_t availableExtCount;
        vkEnumerateDeviceExtensionProperties(m_NativeAdapter, nullptr, &availableExtCount, nullptr);
        festd::small_vector<VkExtensionProperties> availableExt(availableExtCount, VkExtensionProperties{});
        vkEnumerateDeviceExtensionProperties(m_NativeAdapter, nullptr, &availableExtCount, availableExt.data());
        for (auto& ext : RequiredDeviceExtensions)
        {
            const bool found = std::any_of(availableExt.begin(), availableExt.end(), [&](const VkExtensionProperties& props) {
                return StringSlice(ext) == props.extensionName;
            });
            FE_AssertMsg(found, "Vulkan device extension {} was not found", StringSlice(ext));
        }

        constexpr float queuePriority = 1.0f;
        festd::small_vector<VkDeviceQueueCreateInfo> queuesCI{};
        for (auto& queue : m_QueueFamilyIndices)
        {
            auto& queueCI = queuesCI.emplace_back();
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

        return HAL::ResultCode::Success;
    }


    Rc<HAL::CommandQueue> Device::GetCommandQueue(HAL::HardwareQueueKindFlags cmdQueueClass)
    {
        CommandQueueDesc desc{};
        desc.QueueFamilyIndex = GetQueueFamilyIndex(cmdQueueClass);
        desc.QueueIndex = 0;
        return Rc<CommandQueue>::DefaultNew(this, desc);
    }


    void Device::WaitIdle()
    {
        vkDeviceWaitIdle(m_NativeDevice);
    }


    VkSemaphore& Device::AddWaitSemaphore()
    {
        return m_WaitSemaphores.push_back();
    }


    VkSemaphore& Device::AddSignalSemaphore()
    {
        return m_SignalSemaphores.push_back();
    }


    uint32_t Device::GetWaitSemaphores(const VkSemaphore** semaphores)
    {
        *semaphores = m_WaitSemaphores.data();
        return static_cast<uint32_t>(m_WaitSemaphores.size());
    }


    uint32_t Device::GetSignalSemaphores(const VkSemaphore** semaphores)
    {
        *semaphores = m_SignalSemaphores.data();
        return static_cast<uint32_t>(m_SignalSemaphores.size());
    }


    Device::~Device()
    {
        DisposePending();

        for (auto& family : m_QueueFamilyIndices)
        {
            vkDestroyCommandPool(m_NativeDevice, family.CmdPool, VK_NULL_HANDLE);
        }

        vkDestroyDevice(m_NativeDevice, VK_NULL_HANDLE);
    }
} // namespace FE::Graphics::Vulkan
