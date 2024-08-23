#include <FeCore/Console/FeLog.h>
#include <FeCore/Containers/SmallVector.h>
#include <FeCore/DI/Builder.h>

#include <HAL/ShaderCompilerDXC.h>
#include <HAL/Vulkan/Buffer.h>
#include <HAL/Vulkan/CommandList.h>
#include <HAL/Vulkan/DescriptorAllocator.h>
#include <HAL/Vulkan/DescriptorHeap.h>
#include <HAL/Vulkan/Device.h>
#include <HAL/Vulkan/DeviceFactory.h>
#include <HAL/Vulkan/Fence.h>
#include <HAL/Vulkan/Framebuffer.h>
#include <HAL/Vulkan/GraphicsPipeline.h>
#include <HAL/Vulkan/Image.h>
#include <HAL/Vulkan/ImageView.h>
#include <HAL/Vulkan/MemoryRequirementsCache.h>
#include <HAL/Vulkan/RenderPass.h>
#include <HAL/Vulkan/Sampler.h>
#include <HAL/Vulkan/ShaderModule.h>
#include <HAL/Vulkan/Swapchain.h>
#include <HAL/Vulkan/TransientResourceHeap.h>


namespace FE::Graphics::Vulkan
{
#if FE_DEBUG
    static Debug::LogMessageType GetLogMessageType(VkDebugReportFlagsEXT flags)
    {
        switch (static_cast<VkDebugReportFlagBitsEXT>(flags))
        {
        case VK_DEBUG_REPORT_DEBUG_BIT_EXT:
        case VK_DEBUG_REPORT_INFORMATION_BIT_EXT:
            return Debug::LogMessageType::Message;
        case VK_DEBUG_REPORT_WARNING_BIT_EXT:
        case VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT:
            return Debug::LogMessageType::Warning;
        case VK_DEBUG_REPORT_ERROR_BIT_EXT:
            return Debug::LogMessageType::Error;
        default:
            return Debug::LogMessageType::None;
        }
    }


    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugReportCallback(VkDebugReportFlagsEXT flags,
                                                              [[maybe_unused]] VkDebugReportObjectTypeEXT objectType,
                                                              [[maybe_unused]] uint64_t object, [[maybe_unused]] size_t location,
                                                              [[maybe_unused]] int32_t messageCode,
                                                              [[maybe_unused]] const char* pLayerPrefix, const char* pMessage,
                                                              void* pUserData)
    {
        constexpr static auto ignoredMessages = std::array{ "VUID-VkShaderModuleCreateInfo-pCode-04147", "Device Extension:" };

        StringSlice message = pMessage;
        for (auto& msg : ignoredMessages)
        {
            if (message.StartsWith(msg))
            {
                return VK_FALSE;
            }
        }

        const Debug::LogMessageType type = GetLogMessageType(flags);
        static_cast<Debug::IConsoleLogger*>(pUserData)->Log(type, "{}", message);
        return VK_FALSE;
    }
#endif

    inline static HAL::AdapterKind VKConvert(VkPhysicalDeviceType src) noexcept
    {
        switch (src)
        {
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            return HAL::AdapterKind::Integrated;
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            return HAL::AdapterKind::Discrete;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            return HAL::AdapterKind::Virtual;
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            return HAL::AdapterKind::CPU;
        default:
            return HAL::AdapterKind::None;
        }
    }


    DeviceFactory::~DeviceFactory()
    {
        vkDestroyDebugReportCallbackEXT(m_Instance, m_Debug, VK_NULL_HANDLE);
        vkDestroyInstance(m_Instance, VK_NULL_HANDLE);
        FE_LOG_MESSAGE("Vulkan instance was destroyed");
    }


    HAL::ResultCode DeviceFactory::CreateDevice(Env::Name adapterName)
    {
        for (uint32_t adapterIndex = 0; adapterIndex < m_Adapters.size(); ++adapterIndex)
        {
            if (m_Adapters[adapterIndex].Name == adapterName)
            {
                Rc device = Env::GetServiceProvider()->ResolveRequired<HAL::Device>();
                return ImplCast(device.Get())->Init(m_NativeAdapters[adapterIndex]);
            }
        }

        return HAL::ResultCode::UnknownError;
    }


    void DeviceFactory::RegisterServices(DI::ServiceRegistryBuilder& builder)
    {
        builder.Bind<HAL::Device>().To<Device>().InSingletonScope();

        builder.Bind<HAL::Fence>().To<Fence>().InTransientScope();
        builder.Bind<HAL::CommandList>().To<CommandList>().InTransientScope();
        builder.Bind<HAL::Swapchain>().To<Swapchain>().InTransientScope();
        builder.Bind<HAL::Buffer>().To<Buffer>().InTransientScope();
        builder.Bind<HAL::Image>().To<Image>().InTransientScope();
        builder.Bind<HAL::ShaderModule>().To<ShaderModule>().InTransientScope();
        builder.Bind<HAL::RenderPass>().To<RenderPass>().InTransientScope();
        builder.Bind<HAL::DescriptorHeap>().To<DescriptorHeap>().InTransientScope();
        builder.Bind<HAL::GraphicsPipeline>().To<GraphicsPipeline>().InTransientScope();
        builder.Bind<HAL::ImageView>().To<ImageView>().InTransientScope();
        builder.Bind<HAL::Framebuffer>().To<Framebuffer>().InTransientScope();
        builder.Bind<HAL::Sampler>().To<Sampler>().InTransientScope();
        builder.Bind<HAL::TransientResourceHeap>().To<TransientResourceHeap>().InTransientScope();

        builder.Bind<MemoryRequirementsCache>().ToSelf().InSingletonScope();
        builder.Bind<DescriptorAllocator>().ToSelf().InSingletonScope();

        builder.Bind<HAL::ShaderCompiler>().ToConst(Rc<HAL::ShaderCompilerDXC>::DefaultNew(HAL::GraphicsAPI::Vulkan));
    }


    DeviceFactory::DeviceFactory(Env::Configuration* pConfig, Debug::IConsoleLogger* pLogger)
        : m_pLogger(pLogger)
    {
        volkInitialize();
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        festd::small_vector<VkLayerProperties> layers(layerCount, VkLayerProperties{});
        vkEnumerateInstanceLayerProperties(&layerCount, layers.data());
        for (auto& layer : RequiredInstanceLayers)
        {
            auto layerSlice = StringSlice(layer);
            bool found = eastl::any_of(layers.begin(), layers.end(), [&](const VkLayerProperties& props) {
                return layerSlice == props.layerName;
            });
            FE_ASSERT_MSG(found, "Vulkan instance layer {} was not found", layerSlice);
        }

        uint32_t extensionCount;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

        festd::small_vector<VkExtensionProperties> extensions(extensionCount, VkExtensionProperties{});
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
        for (const char* ext : RequiredInstanceExtensions)
        {
            const StringSlice extSlice{ ext };
            const bool found = eastl::any_of(extensions.begin(), extensions.end(), [&](const VkExtensionProperties& props) {
                return extSlice == props.extensionName;
            });
            FE_ASSERT_MSG(found, "Vulkan instance extension {} was not found", extSlice);
        }

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.apiVersion = VK_API_VERSION_1_2;
        appInfo.pEngineName = "Ferrum3D";
        appInfo.pApplicationName = pConfig->GetName("ApplicationName", {}).c_str();

        VkInstanceCreateInfo instanceCI{};
        instanceCI.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceCI.pApplicationInfo = &appInfo;
        instanceCI.enabledLayerCount = static_cast<uint32_t>(RequiredInstanceLayers.size());
        instanceCI.ppEnabledLayerNames = RequiredInstanceLayers.data();
        instanceCI.enabledExtensionCount = static_cast<uint32_t>(RequiredInstanceExtensions.size());
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
        debugCI.pUserData = m_pLogger.Get();
        vkCreateDebugReportCallbackEXT(m_Instance, &debugCI, VK_NULL_HANDLE, &m_Debug);
#endif
        m_pLogger->LogMessage("Vulkan instance created successfully");

        uint32_t adapterCount;
        vkEnumeratePhysicalDevices(m_Instance, &adapterCount, nullptr);

        m_NativeAdapters.resize(adapterCount, VkPhysicalDevice{});
        vkEnumeratePhysicalDevices(m_Instance, &adapterCount, m_NativeAdapters.data());
        for (VkPhysicalDevice& physicalDevice : m_NativeAdapters)
        {
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(physicalDevice, &props);
            m_pLogger->LogMessage("Found Vulkan-compatible GPU: {}", StringSlice(props.deviceName));

            HAL::AdapterInfo& info = m_Adapters.push_back();
            info.Kind = VKConvert(props.deviceType);
            info.Name = Env::Name{ props.deviceName };
        }
    }


    festd::span<const HAL::AdapterInfo> DeviceFactory::EnumerateAdapters() const
    {
        return m_Adapters;
    }
} // namespace FE::Graphics::Vulkan
