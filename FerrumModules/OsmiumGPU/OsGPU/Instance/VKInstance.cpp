#include <FeCore/Console/FeLog.h>
#include <OsGPU/Adapter/VKAdapter.h>
#include <OsGPU/Instance/VKInstance.h>

#if FE_DEBUG
FE::Debug::LogMessageType GetLogMessageType(VkDebugReportFlagsEXT flags)
{
    switch (static_cast<VkDebugReportFlagBitsEXT>(flags))
    {
    case VK_DEBUG_REPORT_DEBUG_BIT_EXT:
    case VK_DEBUG_REPORT_INFORMATION_BIT_EXT:
        return FE::Debug::LogMessageType::Message;
    case VK_DEBUG_REPORT_WARNING_BIT_EXT:
    case VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT:
        return FE::Debug::LogMessageType::Warning;
    case VK_DEBUG_REPORT_ERROR_BIT_EXT:
        return FE::Debug::LogMessageType::Error;
    default:
        return FE::Debug::LogMessageType::None;
    }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugReportCallback(VkDebugReportFlagsEXT flags,
                                                          [[maybe_unused]] VkDebugReportObjectTypeEXT objectType,
                                                          [[maybe_unused]] FE::UInt64 object, [[maybe_unused]] size_t location,
                                                          [[maybe_unused]] FE::Int32 messageCode,
                                                          [[maybe_unused]] const char* pLayerPrefix, const char* pMessage,
                                                          void* pUserData)
{
    constexpr static auto ignoredMessages = std::array{ "VUID-VkShaderModuleCreateInfo-pCode-04147", "Device Extension:" };

    FE::StringSlice message = pMessage;
    for (auto& msg : ignoredMessages)
    {
        if (message.StartsWith(msg))
        {
            return VK_FALSE;
        }
    }

    auto type = GetLogMessageType(flags);
    static_cast<FE::Debug::IConsoleLogger*>(pUserData)->Log(type, "{}", message);
    return VK_FALSE;
}
#endif

namespace FE::Osmium
{
    VKInstance::~VKInstance()
    {
        vkDestroyDebugReportCallbackEXT(m_Instance, m_Debug, VK_NULL_HANDLE);
        vkDestroyInstance(m_Instance, VK_NULL_HANDLE);
        FE_LOG_MESSAGE("Vulkan instance was destroyed");
    }

    VKInstance::VKInstance(const InstanceDesc& desc)
    {
        volkInitialize();
        UInt32 layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        List<VkLayerProperties> layers(layerCount, VkLayerProperties{});
        vkEnumerateInstanceLayerProperties(&layerCount, layers.Data());
        for (auto& layer : RequiredInstanceLayers)
        {
            auto layerSlice = StringSlice(layer);
            bool found      = std::any_of(layers.begin(), layers.end(), [&](const VkLayerProperties& props) {
                return layerSlice == props.layerName;
            });
            FE_ASSERT_MSG(found, "Vulkan instance layer {} was not found", layerSlice);
        }

        UInt32 extensionCount;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        List<VkExtensionProperties> extensions(extensionCount, VkExtensionProperties{});
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.Data());
        for (auto& ext : RequiredInstanceExtensions)
        {
            auto extSlice = StringSlice(ext);
            bool found    = std::any_of(extensions.begin(), extensions.end(), [&](const VkExtensionProperties& props) {
                return extSlice == props.extensionName;
            });
            FE_ASSERT_MSG(found, "Vulkan instance extension {} was not found", extSlice);
        }

        VkApplicationInfo appInfo{};
        appInfo.sType            = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.apiVersion       = VK_API_VERSION_1_2;
        appInfo.pEngineName      = FerrumEngineName;
        appInfo.pApplicationName = desc.ApplicationName;

        VkInstanceCreateInfo instanceCI{};
        instanceCI.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceCI.pApplicationInfo        = &appInfo;
        instanceCI.enabledLayerCount       = static_cast<UInt32>(RequiredInstanceLayers.size());
        instanceCI.ppEnabledLayerNames     = RequiredInstanceLayers.data();
        instanceCI.enabledExtensionCount   = static_cast<UInt32>(RequiredInstanceExtensions.size());
        instanceCI.ppEnabledExtensionNames = RequiredInstanceExtensions.data();

        vkCreateInstance(&instanceCI, VK_NULL_HANDLE, &m_Instance);
        volkLoadInstance(m_Instance);
#if FE_DEBUG
        VkDebugReportCallbackCreateInfoEXT debugCI{};
        debugCI.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        debugCI.flags |= VK_DEBUG_REPORT_WARNING_BIT_EXT;
        debugCI.flags |= VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
        debugCI.flags |= VK_DEBUG_REPORT_ERROR_BIT_EXT;
        debugCI.flags |= VK_DEBUG_REPORT_DEBUG_BIT_EXT;
        debugCI.pfnCallback = &DebugReportCallback;
        debugCI.pUserData   = FE::ServiceLocator<FE::Debug::IConsoleLogger>::Get();
        vkCreateDebugReportCallbackEXT(m_Instance, &debugCI, VK_NULL_HANDLE, &m_Debug);
#endif
        FE_LOG_MESSAGE("Vulkan instance created successfully");

        UInt32 adapterCount;
        vkEnumeratePhysicalDevices(m_Instance, &adapterCount, nullptr);
        List<VkPhysicalDevice> vkAdapters(adapterCount, VkPhysicalDevice{});
        vkEnumeratePhysicalDevices(m_Instance, &adapterCount, vkAdapters.Data());
        for (auto& vkAdapter : vkAdapters)
        {
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(vkAdapter, &props);
            FE_LOG_MESSAGE("Found Vulkan-compatible GPU: {}", StringSlice(props.deviceName));
            m_Adapters.Push(MakeShared<VKAdapter>(*this, vkAdapter));
        }
    }

    VkInstance VKInstance::GetNativeInstance()
    {
        return m_Instance;
    }

    const List<Rc<IAdapter>>& VKInstance::GetAdapters() const
    {
        return m_Adapters;
    }
} // namespace FE::Osmium
