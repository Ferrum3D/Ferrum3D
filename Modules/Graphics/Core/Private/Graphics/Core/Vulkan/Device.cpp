#include <FeCore/Logging/Trace.h>
#include <Graphics/Core/DeviceObject.h>
#include <Graphics/Core/ShaderCompilerDXC.h>
#include <Graphics/Core/Vulkan/Base/BaseTypes.h>
#include <Graphics/Core/Vulkan/Device.h>
#include <Graphics/Core/Vulkan/DeviceFactory.h>
#include <festd/vector.h>

namespace FE::Graphics::Vulkan
{
    constexpr auto kRequiredDeviceExtensions = std::array{ VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                                                           VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
                                                           VK_EXT_MUTABLE_DESCRIPTOR_TYPE_EXTENSION_NAME,
                                                           VK_EXT_MESH_SHADER_EXTENSION_NAME,
                                                           VK_KHR_SPIRV_1_4_EXTENSION_NAME,
                                                           VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME,
                                                           VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME };


    void Device::FindQueueFamilies()
    {
        FE_PROFILER_ZONE();

        const auto hasQueueFamily = [this](const Core::HardwareQueueKindFlags cmdQueueClass) {
            return std::any_of(m_queueFamilyIndices.begin(), m_queueFamilyIndices.end(), [=](const QueueFamilyData& data) {
                return data.m_class == cmdQueueClass;
            });
        };

        uint32_t familyCount;
        vkGetPhysicalDeviceQueueFamilyProperties(m_nativeAdapter, &familyCount, nullptr);

        festd::inline_vector<VkQueueFamilyProperties> families(familyCount, VkQueueFamilyProperties{});
        vkGetPhysicalDeviceQueueFamilyProperties(m_nativeAdapter, &familyCount, families.data());
        for (uint32_t i = 0; i < families.size(); ++i)
        {
            const uint32_t kGraphics = VK_QUEUE_TRANSFER_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT;
            const uint32_t kCompute = VK_QUEUE_TRANSFER_BIT | VK_QUEUE_COMPUTE_BIT;
            const uint32_t kCopy = VK_QUEUE_TRANSFER_BIT;

            const uint32_t idx = static_cast<uint32_t>(i);
            if ((families[i].queueFlags & kGraphics) == kGraphics && !hasQueueFamily(Core::HardwareQueueKindFlags::kGraphics))
            {
                m_queueFamilyIndices.push_back(
                    QueueFamilyData(idx, families[i].queueCount, Core::HardwareQueueKindFlags::kGraphics));
            }
            else if ((families[i].queueFlags & kCompute) == kCompute && !hasQueueFamily(Core::HardwareQueueKindFlags::kCompute))
            {
                m_queueFamilyIndices.push_back(
                    QueueFamilyData(idx, families[i].queueCount, Core::HardwareQueueKindFlags::kCompute));
            }
            else if ((families[i].queueFlags & kCopy) == kCopy && !hasQueueFamily(Core::HardwareQueueKindFlags::kTransfer))
            {
                m_queueFamilyIndices.push_back(
                    QueueFamilyData(idx, families[i].queueCount, Core::HardwareQueueKindFlags::kTransfer));
            }
        }
    }


    uint32_t Device::FindMemoryType(const uint32_t typeBits, const VkMemoryPropertyFlags properties) const
    {
        FE_PROFILER_ZONE();

        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(m_nativeAdapter, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
        {
            if ((typeBits & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }

        FE_Assert(false, "Memory type not found");
        return kInvalidIndex;
    }


    Device::Device(Logger* logger, Core::DeviceFactory* factory)
        : Common::Device(logger)
        , m_deviceFactory(ImplCast(factory))
    {
    }


    void Device::Init(VkPhysicalDevice nativeAdapter)
    {
        FE_PROFILER_ZONE();

        FE_Assert(m_nativeAdapter == VK_NULL_HANDLE);
        FE_Assert(m_nativeDevice == VK_NULL_HANDLE);

        m_nativeAdapter = nativeAdapter;

        vkGetPhysicalDeviceProperties(nativeAdapter, &m_adapterProperties);
        m_logger->LogInfo("Creating Vulkan Device on GPU: {}...", m_adapterProperties.deviceName);

        FindQueueFamilies();

        uint32_t availableExtCount;
        vkEnumerateDeviceExtensionProperties(m_nativeAdapter, nullptr, &availableExtCount, nullptr);
        festd::inline_vector<VkExtensionProperties> availableExt(availableExtCount, VkExtensionProperties{});
        vkEnumerateDeviceExtensionProperties(m_nativeAdapter, nullptr, &availableExtCount, availableExt.data());
        for (const char* extension : kRequiredDeviceExtensions)
        {
            const bool found = std::any_of(availableExt.begin(), availableExt.end(), [&](const VkExtensionProperties& props) {
                return festd::string_view(extension) == props.extensionName;
            });
            FE_AssertMsg(found, "Vulkan device extension {} was not found", festd::string_view(extension));
        }

        constexpr float queuePriority = 1.0f;
        festd::inline_vector<VkDeviceQueueCreateInfo> queuesCI{};
        for (auto& queue : m_queueFamilyIndices)
        {
            auto& queueCI = queuesCI.emplace_back();
            queueCI.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCI.queueFamilyIndex = queue.m_familyIndex;
            queueCI.queueCount = 1;
            queueCI.pQueuePriorities = &queuePriority;
        }

        VkPhysicalDeviceMeshShaderFeaturesEXT meshShaderFeatures{};
        meshShaderFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
        meshShaderFeatures.pNext = nullptr;
        meshShaderFeatures.meshShader = true;
        meshShaderFeatures.taskShader = true;

        VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT mutableDescriptorTypeFeatures{};
        mutableDescriptorTypeFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MUTABLE_DESCRIPTOR_TYPE_FEATURES_EXT;
        mutableDescriptorTypeFeatures.pNext = &meshShaderFeatures;
        mutableDescriptorTypeFeatures.mutableDescriptorType = true;

        VkPhysicalDeviceVulkan13Features deviceFeatures13{};
        deviceFeatures13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
        deviceFeatures13.pNext = &mutableDescriptorTypeFeatures;
        deviceFeatures13.synchronization2 = true;
        deviceFeatures13.dynamicRendering = true;

        VkPhysicalDeviceVulkan12Features deviceFeatures12{};
        deviceFeatures12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        deviceFeatures12.pNext = &deviceFeatures13;
        deviceFeatures12.timelineSemaphore = true;
        deviceFeatures12.scalarBlockLayout = true;
        deviceFeatures12.descriptorIndexing = true;
        deviceFeatures12.runtimeDescriptorArray = true;
        deviceFeatures12.descriptorBindingPartiallyBound = true;
        deviceFeatures12.descriptorBindingVariableDescriptorCount = true;
        deviceFeatures12.descriptorBindingSampledImageUpdateAfterBind = true;
        deviceFeatures12.descriptorBindingStorageImageUpdateAfterBind = true;
        deviceFeatures12.descriptorBindingStorageBufferUpdateAfterBind = true;

        deviceFeatures12.shaderSampledImageArrayNonUniformIndexing = true;
        deviceFeatures12.shaderStorageBufferArrayNonUniformIndexing = true;
        deviceFeatures12.shaderStorageImageArrayNonUniformIndexing = true;
        deviceFeatures12.shaderUniformTexelBufferArrayNonUniformIndexing = true;
        deviceFeatures12.shaderStorageTexelBufferArrayNonUniformIndexing = true;

        VkPhysicalDeviceFeatures2 deviceFeatures2{};
        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures2.pNext = &deviceFeatures12;

        VkPhysicalDeviceFeatures& deviceFeatures = deviceFeatures2.features;
        deviceFeatures.geometryShader = true;
        deviceFeatures.tessellationShader = true;
        deviceFeatures.samplerAnisotropy = true;
        deviceFeatures.sampleRateShading = true;
        deviceFeatures.shaderImageGatherExtended = true;

        VkDeviceCreateInfo deviceCI{};
        deviceCI.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCI.pNext = &deviceFeatures2;
        deviceCI.queueCreateInfoCount = queuesCI.size();
        deviceCI.pQueueCreateInfos = queuesCI.data();
        deviceCI.enabledExtensionCount = static_cast<uint32_t>(kRequiredDeviceExtensions.size());
        deviceCI.ppEnabledExtensionNames = kRequiredDeviceExtensions.data();

        VerifyVulkan(vkCreateDevice(m_nativeAdapter, &deviceCI, VK_NULL_HANDLE, &m_nativeDevice));
        volkLoadDevice(m_nativeDevice);

        for (auto& queueFamilyData : m_queueFamilyIndices)
        {
            VkCommandPoolCreateInfo poolCI{};
            poolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            poolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            poolCI.queueFamilyIndex = queueFamilyData.m_familyIndex;
            vkCreateCommandPool(m_nativeDevice, &poolCI, VK_NULL_HANDLE, &queueFamilyData.m_commandPool);
        }

        m_samplerCache.Init(this);
    }


    void Device::WaitIdle()
    {
        FE_PROFILER_ZONE();

        vkDeviceWaitIdle(m_nativeDevice);
        ForceReleasePendingDisposers();
    }


    Device::~Device()
    {
        ForceReleasePendingDisposers();

        m_samplerCache.Shutdown();

        for (const auto& family : m_queueFamilyIndices)
            vkDestroyCommandPool(m_nativeDevice, family.m_commandPool, VK_NULL_HANDLE);

        m_queueFamilyIndices.clear();

        vkDestroyDevice(m_nativeDevice, VK_NULL_HANDLE);
        m_nativeDevice = VK_NULL_HANDLE;
    }
} // namespace FE::Graphics::Vulkan
