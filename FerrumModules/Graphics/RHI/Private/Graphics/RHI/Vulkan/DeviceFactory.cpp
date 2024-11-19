#include <FeCore/Containers/SmallVector.h>
#include <FeCore/DI/Builder.h>
#include <FeCore/Logging/Trace.h>

#include <Graphics/RHI/ShaderCompilerDXC.h>
#include <Graphics/RHI/Vulkan/Buffer.h>
#include <Graphics/RHI/Vulkan/CommandList.h>
#include <Graphics/RHI/Vulkan/DescriptorAllocator.h>
#include <Graphics/RHI/Vulkan/Device.h>
#include <Graphics/RHI/Vulkan/DeviceFactory.h>
#include <Graphics/RHI/Vulkan/Fence.h>
#include <Graphics/RHI/Vulkan/Framebuffer.h>
#include <Graphics/RHI/Vulkan/GraphicsPipeline.h>
#include <Graphics/RHI/Vulkan/Image.h>
#include <Graphics/RHI/Vulkan/ImageView.h>
#include <Graphics/RHI/Vulkan/MemoryRequirementsCache.h>
#include <Graphics/RHI/Vulkan/RenderPass.h>
#include <Graphics/RHI/Vulkan/Sampler.h>
#include <Graphics/RHI/Vulkan/ShaderModule.h>
#include <Graphics/RHI/Vulkan/ShaderResourceGroup.h>
#include <Graphics/RHI/Vulkan/Swapchain.h>
#include <Graphics/RHI/Vulkan/TransientResourceHeap.h>


namespace FE::Graphics::Vulkan
{
#if FE_DEBUG
    static LogSeverity GetLogMessageType(VkDebugReportFlagsEXT flags)
    {
        switch (static_cast<VkDebugReportFlagBitsEXT>(flags))
        {
        case VK_DEBUG_REPORT_DEBUG_BIT_EXT:
        case VK_DEBUG_REPORT_INFORMATION_BIT_EXT:
            return LogSeverity::kInfo;
        case VK_DEBUG_REPORT_WARNING_BIT_EXT:
        case VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT:
            return LogSeverity::kWarning;
        case VK_DEBUG_REPORT_ERROR_BIT_EXT:
            return LogSeverity::kError;
        default:
            return LogSeverity::kInfo;
        }
    }


    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugReportCallback(VkDebugReportFlagsEXT flags,
                                                              [[maybe_unused]] VkDebugReportObjectTypeEXT objectType,
                                                              [[maybe_unused]] uint64_t object, [[maybe_unused]] size_t location,
                                                              [[maybe_unused]] int32_t messageCode,
                                                              [[maybe_unused]] const char* pLayerPrefix, const char* pMessage,
                                                              void* pUserData)
    {
        constexpr static auto ignoredMessages =
            std::array{ "Validation Error: [ VUID-VkShaderModuleCreateInfo-pCode-08742", "Device Extension:" };

        const StringSlice message = pMessage;
        for (auto& msg : ignoredMessages)
        {
            if (message.StartsWith(msg))
                return VK_FALSE;
        }

        const LogSeverity type = GetLogMessageType(flags);
        static_cast<Logger*>(pUserData)->Log(type, "{}", message);

#    if FE_DEBUG
        if (type == LogSeverity::kError)
            FE_DebugBreak();
#    endif

        return VK_FALSE;
    }
#endif


    inline static RHI::AdapterKind VKConvert(VkPhysicalDeviceType src) noexcept
    {
        switch (src)
        {
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            return RHI::AdapterKind::kIntegrated;
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            return RHI::AdapterKind::kDiscrete;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            return RHI::AdapterKind::kVirtual;
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            return RHI::AdapterKind::kCPU;
        default:
            return RHI::AdapterKind::kNone;
        }
    }


    DeviceFactory::~DeviceFactory()
    {
        vkDestroyDebugReportCallbackEXT(m_instance, m_debug, VK_NULL_HANDLE);
        vkDestroyInstance(m_instance, VK_NULL_HANDLE);
        m_logger->LogInfo("Vulkan instance was destroyed");
    }


    RHI::ResultCode DeviceFactory::CreateDevice(Env::Name adapterName)
    {
        for (uint32_t adapterIndex = 0; adapterIndex < m_adapters.size(); ++adapterIndex)
        {
            if (m_adapters[adapterIndex].m_name == adapterName)
            {
                Rc device = Env::GetServiceProvider()->ResolveRequired<RHI::Device>();
                return ImplCast(device.Get())->Init(m_nativeAdapters[adapterIndex]);
            }
        }

        return RHI::ResultCode::kUnknownError;
    }


    void DeviceFactory::RegisterServices(DI::ServiceRegistryBuilder& builder)
    {
        builder.Bind<RHI::Device>().To<Device>().InSingletonScope();

        builder.Bind<RHI::Fence>().To<Fence>().InTransientScope();
        builder.Bind<RHI::CommandList>().To<CommandList>().InTransientScope();
        builder.Bind<RHI::Swapchain>().To<Swapchain>().InTransientScope();
        builder.Bind<RHI::Buffer>().To<Buffer>().InTransientScope();
        builder.Bind<RHI::Image>().To<Image>().InTransientScope();
        builder.Bind<RHI::ShaderModule>().To<ShaderModule>().InTransientScope();
        builder.Bind<RHI::RenderPass>().To<RenderPass>().InTransientScope();
        builder.Bind<RHI::GraphicsPipeline>().To<GraphicsPipeline>().InTransientScope();
        builder.Bind<RHI::ImageView>().To<ImageView>().InTransientScope();
        builder.Bind<RHI::Framebuffer>().To<Framebuffer>().InTransientScope();
        builder.Bind<RHI::Sampler>().To<Sampler>().InTransientScope();
        builder.Bind<RHI::TransientResourceHeap>().To<TransientResourceHeap>().InTransientScope();
        builder.Bind<RHI::ShaderResourceGroup>().To<ShaderResourceGroup>().InTransientScope();

        builder.Bind<MemoryRequirementsCache>().ToSelf().InSingletonScope();
        builder.Bind<DescriptorAllocator>().ToSelf().InSingletonScope();

        builder.Bind<RHI::ShaderCompiler>().ToConst(
            Rc<RHI::ShaderCompilerDXC>::DefaultNew(m_logger.Get(), RHI::GraphicsAPI::kVulkan));
    }


    DeviceFactory::DeviceFactory(Env::Configuration* config, Logger* logger)
        : m_logger(logger)
    {
        volkInitialize();
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        festd::small_vector<VkLayerProperties> layers(layerCount, VkLayerProperties{});
        vkEnumerateInstanceLayerProperties(&layerCount, layers.data());
        for (const char* layer : kRequiredInstanceLayers)
        {
            const StringSlice layerSlice{ layer };
            const bool found = eastl::any_of(layers.begin(), layers.end(), [&](const VkLayerProperties& props) {
                return layerSlice == props.layerName;
            });
            FE_AssertMsg(found, "Vulkan instance layer {} was not found", layerSlice);
        }

        uint32_t extensionCount;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

        festd::small_vector<VkExtensionProperties> extensions(extensionCount, VkExtensionProperties{});
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
        for (const char* ext : kRequiredInstanceExtensions)
        {
            const StringSlice extSlice{ ext };
            const bool found = eastl::any_of(extensions.begin(), extensions.end(), [&](const VkExtensionProperties& props) {
                return extSlice == props.extensionName;
            });
            FE_AssertMsg(found, "Vulkan instance extension {} was not found", extSlice);
        }

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.apiVersion = VK_API_VERSION_1_2;
        appInfo.pEngineName = "Ferrum3D";
        appInfo.pApplicationName = config->GetName("ApplicationName", {}).c_str();

        VkInstanceCreateInfo instanceCI{};
        instanceCI.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceCI.pApplicationInfo = &appInfo;
        instanceCI.enabledLayerCount = static_cast<uint32_t>(kRequiredInstanceLayers.size());
        instanceCI.ppEnabledLayerNames = kRequiredInstanceLayers.data();
        instanceCI.enabledExtensionCount = static_cast<uint32_t>(kRequiredInstanceExtensions.size());
        instanceCI.ppEnabledExtensionNames = kRequiredInstanceExtensions.data();

        vkCreateInstance(&instanceCI, VK_NULL_HANDLE, &m_instance);
        volkLoadInstance(m_instance);

#if FE_DEBUG
        VkDebugReportCallbackCreateInfoEXT debugCI{};
        debugCI.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        debugCI.flags |= VK_DEBUG_REPORT_WARNING_BIT_EXT;
        debugCI.flags |= VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
        debugCI.flags |= VK_DEBUG_REPORT_ERROR_BIT_EXT;
        debugCI.flags |= VK_DEBUG_REPORT_DEBUG_BIT_EXT;
        debugCI.pfnCallback = &DebugReportCallback;
        debugCI.pUserData = m_logger.Get();
        vkCreateDebugReportCallbackEXT(m_instance, &debugCI, VK_NULL_HANDLE, &m_debug);
#endif
        m_logger->LogInfo("Vulkan instance created successfully");

        uint32_t adapterCount;
        vkEnumeratePhysicalDevices(m_instance, &adapterCount, nullptr);

        m_nativeAdapters.resize(adapterCount, VkPhysicalDevice{});
        vkEnumeratePhysicalDevices(m_instance, &adapterCount, m_nativeAdapters.data());
        for (VkPhysicalDevice& physicalDevice : m_nativeAdapters)
        {
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(physicalDevice, &props);
            m_logger->LogInfo("Found Vulkan-compatible GPU: {}", StringSlice(props.deviceName));

            RHI::AdapterInfo& info = m_adapters.push_back();
            info.m_kind = VKConvert(props.deviceType);
            info.m_name = Env::Name{ props.deviceName };
        }
    }


    festd::span<const RHI::AdapterInfo> DeviceFactory::EnumerateAdapters() const
    {
        return m_adapters;
    }
} // namespace FE::Graphics::Vulkan
