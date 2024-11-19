#include <FeCore/Containers/SmallVector.h>
#include <FeCore/Logging/Trace.h>
#include <Graphics/RHI/DeviceObject.h>
#include <Graphics/RHI/Image.h>
#include <Graphics/RHI/ShaderCompilerDXC.h>
#include <Graphics/RHI/Vulkan/CommandQueue.h>
#include <Graphics/RHI/Vulkan/Common/BaseTypes.h>
#include <Graphics/RHI/Vulkan/Device.h>
#include <Graphics/RHI/Vulkan/DeviceFactory.h>
#include <Graphics/RHI/Window.h>
#include <algorithm>

namespace FE::Graphics::Vulkan
{
    constexpr auto kRequiredDeviceExtensions = std::array{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };


    void Device::FindQueueFamilies()
    {
        const auto hasQueueFamily = [this](RHI::HardwareQueueKindFlags cmdQueueClass) {
            return std::any_of(m_queueFamilyIndices.begin(), m_queueFamilyIndices.end(), [=](const QueueFamilyData& data) {
                return data.m_class == cmdQueueClass;
            });
        };

        uint32_t familyCount;
        vkGetPhysicalDeviceQueueFamilyProperties(m_nativeAdapter, &familyCount, nullptr);

        festd::small_vector<VkQueueFamilyProperties> families(familyCount, VkQueueFamilyProperties{});
        vkGetPhysicalDeviceQueueFamilyProperties(m_nativeAdapter, &familyCount, families.data());
        for (uint32_t i = 0; i < families.size(); ++i)
        {
            const uint32_t kGraphics = VK_QUEUE_TRANSFER_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT;
            const uint32_t kCompute = VK_QUEUE_TRANSFER_BIT | VK_QUEUE_COMPUTE_BIT;
            const uint32_t kCopy = VK_QUEUE_TRANSFER_BIT;

            const uint32_t idx = static_cast<uint32_t>(i);
            if ((families[i].queueFlags & kGraphics) == kGraphics && !hasQueueFamily(RHI::HardwareQueueKindFlags::kGraphics))
            {
                m_queueFamilyIndices.push_back(
                    QueueFamilyData(idx, families[i].queueCount, RHI::HardwareQueueKindFlags::kGraphics));
            }
            else if ((families[i].queueFlags & kCompute) == kCompute && !hasQueueFamily(RHI::HardwareQueueKindFlags::kCompute))
            {
                m_queueFamilyIndices.push_back(
                    QueueFamilyData(idx, families[i].queueCount, RHI::HardwareQueueKindFlags::kCompute));
            }
            else if ((families[i].queueFlags & kCopy) == kCopy && !hasQueueFamily(RHI::HardwareQueueKindFlags::kTransfer))
            {
                m_queueFamilyIndices.push_back(
                    QueueFamilyData(idx, families[i].queueCount, RHI::HardwareQueueKindFlags::kTransfer));
            }
        }
    }


    uint32_t Device::FindMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(m_nativeAdapter, &memProperties);

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


    Device::Device(Logger* logger, RHI::DeviceFactory* factory)
        : RHI::Device(logger)
        , m_deviceFactory(ImplCast(factory))
    {
    }


    RHI::ResultCode Device::Init(VkPhysicalDevice nativeAdapter)
    {
        m_nativeAdapter = nativeAdapter;

        vkGetPhysicalDeviceProperties(nativeAdapter, &m_adapterProperties);
        m_logger->LogInfo("Creating Vulkan Device on GPU: {}...", m_adapterProperties.deviceName);

        FindQueueFamilies();

        uint32_t availableExtCount;
        vkEnumerateDeviceExtensionProperties(m_nativeAdapter, nullptr, &availableExtCount, nullptr);
        festd::small_vector<VkExtensionProperties> availableExt(availableExtCount, VkExtensionProperties{});
        vkEnumerateDeviceExtensionProperties(m_nativeAdapter, nullptr, &availableExtCount, availableExt.data());
        for (const char* extension : kRequiredDeviceExtensions)
        {
            const bool found = std::any_of(availableExt.begin(), availableExt.end(), [&](const VkExtensionProperties& props) {
                return StringSlice(extension) == props.extensionName;
            });
            FE_AssertMsg(found, "Vulkan device extension {} was not found", StringSlice(extension));
        }

        constexpr float queuePriority = 1.0f;
        festd::small_vector<VkDeviceQueueCreateInfo> queuesCI{};
        for (auto& queue : m_queueFamilyIndices)
        {
            auto& queueCI = queuesCI.emplace_back();
            queueCI.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCI.queueFamilyIndex = queue.m_familyIndex;
            queueCI.queueCount = 1;
            queueCI.pQueuePriorities = &queuePriority;
        }

        VkPhysicalDeviceVulkan12Features deviceFeatures12{};
        deviceFeatures12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        deviceFeatures12.timelineSemaphore = true;

        VkPhysicalDeviceFeatures2 deviceFeatures2{};
        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures2.pNext = &deviceFeatures12;

        VkPhysicalDeviceFeatures& deviceFeatures = deviceFeatures2.features;
        deviceFeatures.geometryShader = true;
        deviceFeatures.tessellationShader = true;
        deviceFeatures.samplerAnisotropy = true;
        deviceFeatures.sampleRateShading = true;

        VkDeviceCreateInfo deviceCI{};
        deviceCI.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCI.pNext = &deviceFeatures2;
        deviceCI.queueCreateInfoCount = static_cast<uint32_t>(queuesCI.size());
        deviceCI.pQueueCreateInfos = queuesCI.data();
        deviceCI.enabledExtensionCount = static_cast<uint32_t>(kRequiredDeviceExtensions.size());
        deviceCI.ppEnabledExtensionNames = kRequiredDeviceExtensions.data();

        vkCreateDevice(m_nativeAdapter, &deviceCI, VK_NULL_HANDLE, &m_nativeDevice);
        volkLoadDevice(m_nativeDevice);

        for (auto& queueFamilyData : m_queueFamilyIndices)
        {
            VkCommandPoolCreateInfo poolCI{};
            poolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            poolCI.queueFamilyIndex = queueFamilyData.m_familyIndex;
            vkCreateCommandPool(m_nativeDevice, &poolCI, VK_NULL_HANDLE, &queueFamilyData.m_commandPool);
        }

        return RHI::ResultCode::kSuccess;
    }


    Rc<RHI::CommandQueue> Device::GetCommandQueue(RHI::HardwareQueueKindFlags cmdQueueClass)
    {
        CommandQueueDesc desc{};
        desc.m_queueFamilyIndex = GetQueueFamilyIndex(cmdQueueClass);
        desc.m_queueIndex = 0;
        return Rc<CommandQueue>::DefaultNew(this, desc);
    }


    void Device::WaitIdle()
    {
        vkDeviceWaitIdle(m_nativeDevice);
    }


    Device::~Device()
    {
        DisposePending();

        for (auto& family : m_queueFamilyIndices)
            vkDestroyCommandPool(m_nativeDevice, family.m_commandPool, VK_NULL_HANDLE);

        vkDestroyDevice(m_nativeDevice, VK_NULL_HANDLE);
    }
} // namespace FE::Graphics::Vulkan
