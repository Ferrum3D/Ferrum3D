#include <FeCore/Console/FeLog.h>
#include <GPU/Adapter/VKAdapter.h>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

#if FE_DEBUG
static VKAPI_ATTR VkBool32 VKAPI_CALL DebugReportCallback(
    VkDebugReportFlagsEXT flags, [[maybe_unused]] VkDebugReportObjectTypeEXT objectType, [[maybe_unused]] FE::UInt64 object,
    [[maybe_unused]] size_t location, [[maybe_unused]] FE::Int32 messageCode, [[maybe_unused]] const char* pLayerPrefix,
    const char* pMessage, void* pUserData)
{
    constexpr static auto ignoredMessages = std::array{ "VUID-VkShaderModuleCreateInfo-pCode-04147" };

    std::string_view message = pMessage;
    for (auto& msg : ignoredMessages)
    {
        if (message.find(msg) != std::string_view::npos)
        {
            return VK_FALSE;
        }
    }

    FE::Debug::LogMessageType type = FE::Debug::LogMessageType::Message;
    switch (static_cast<vk::DebugReportFlagBitsEXT>(flags))
    {
    case vk::DebugReportFlagBitsEXT::eInformation:
    case vk::DebugReportFlagBitsEXT::eDebug:
        type = FE::Debug::LogMessageType::Message;
        break;
    case vk::DebugReportFlagBitsEXT::eWarning:
    case vk::DebugReportFlagBitsEXT::ePerformanceWarning:
        type = FE::Debug::LogMessageType::Warning;
        break;
    case vk::DebugReportFlagBitsEXT::eError:
        type = FE::Debug::LogMessageType::Error;
        break;
    default:
        break;
    }

    static_cast<FE::Debug::IConsoleLogger*>(pUserData)->Log(type, pMessage);
    return VK_FALSE;
}
#endif

namespace FE::GPU
{
    VKInstance::VKInstance([[maybe_unused]] const InstanceDesc& desc)
    {
        auto vkGetInstanceProcAddr = m_Loader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
        VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

        auto layers = vk::enumerateInstanceLayerProperties<StdHeapAllocator<vk::LayerProperties>>();
        for (auto& layer : RequiredInstanceLayers)
        {
            bool found = std::any_of(layers.begin(), layers.end(), [&](const vk::LayerProperties& props) {
                return StringSlice(layer) == props.layerName.data();
            });
            FE_ASSERT_MSG(found, "Vulkan instance layer {} was not found", layer);
        }

        auto extensions = vk::enumerateInstanceExtensionProperties<StdHeapAllocator<vk::ExtensionProperties>>();
        for (auto& ext : RequiredInstanceExtensions)
        {
            bool found = std::any_of(extensions.begin(), extensions.end(), [&](const vk::ExtensionProperties& props) {
                return StringSlice(ext) == props.extensionName.data();
            });
            FE_ASSERT_MSG(found, "Vulkan instance extension {} was not found", ext);
        }

        vk::ApplicationInfo appInfo{};
        appInfo.apiVersion = VK_API_VERSION_1_2;
        appInfo.pEngineName = FerrumEngineName;
        appInfo.pApplicationName = desc.ApplicationName;

        vk::InstanceCreateInfo instanceCI{};
        instanceCI.pApplicationInfo        = &appInfo;
        instanceCI.enabledLayerCount       = static_cast<UInt32>(RequiredInstanceLayers.size());
        instanceCI.ppEnabledLayerNames     = RequiredInstanceLayers.data();
        instanceCI.enabledExtensionCount   = static_cast<UInt32>(RequiredInstanceExtensions.size());
        instanceCI.ppEnabledExtensionNames = RequiredInstanceExtensions.data();

        m_Instance = vk::createInstanceUnique(instanceCI);
        VULKAN_HPP_DEFAULT_DISPATCHER.init(m_Instance.get());
#if FE_DEBUG
        vk::DebugReportCallbackCreateInfoEXT debugCI{};
        debugCI.flags |= vk::DebugReportFlagBitsEXT::eWarning;
        debugCI.flags |= vk::DebugReportFlagBitsEXT::ePerformanceWarning;
        debugCI.flags |= vk::DebugReportFlagBitsEXT::eError;
        debugCI.flags |= vk::DebugReportFlagBitsEXT::eDebug;
        debugCI.pfnCallback = &DebugReportCallback;
        debugCI.pUserData   = FE::Singleton<FE::Debug::IConsoleLogger>::Get();
        m_Debug             = m_Instance->createDebugReportCallbackEXTUnique(debugCI);
#endif
        FE_LOG_MESSAGE("Vulkan instance created successfully");

        auto vkAdapters = m_Instance->enumeratePhysicalDevices<StdHeapAllocator<vk::PhysicalDevice>>();
        for (auto& vkAdapter : vkAdapters)
        {
            auto props = vkAdapter.getProperties();
            FE_LOG_MESSAGE("Found Vulkan-compatible GPU: {}", props.deviceName);
            m_PhysicalDevices.push_back(static_pointer_cast<IAdapter>(MakeShared<VKAdapter>(*this, vkAdapter)));
        }
    }

    vk::Instance& VKInstance::GetNativeInstance()
    {
        return m_Instance.get();
    }

    Vector<Shared<IAdapter>>& VKInstance::GetAdapters()
    {
        return m_PhysicalDevices;
    }
} // namespace FE::GPU
