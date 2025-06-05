#include <FeCore/DI/Builder.h>
#include <FeCore/IO/IAsyncStreamIO.h>
#include <FeCore/Logging/Trace.h>
#include <festd/vector.h>

#include <Graphics/Core/Common/ShaderSourceCache.h>
#include <Graphics/Core/ShaderCompilerDXC.h>
#include <Graphics/Core/Vulkan/AsyncCopyQueue.h>
#include <Graphics/Core/Vulkan/DescriptorAllocator.h>
#include <Graphics/Core/Vulkan/Device.h>
#include <Graphics/Core/Vulkan/DeviceFactory.h>
#include <Graphics/Core/Vulkan/Fence.h>
#include <Graphics/Core/Vulkan/FrameGraph/FrameGraph.h>
#include <Graphics/Core/Vulkan/GeometryPool.h>
#include <Graphics/Core/Vulkan/PipelineFactory.h>
#include <Graphics/Core/Vulkan/ResourcePool.h>
#include <Graphics/Core/Vulkan/ShaderLibrary.h>
#include <Graphics/Core/Vulkan/Viewport.h>

namespace FE::Graphics::Vulkan
{
    namespace
    {
#if FE_DEVELOPMENT
        LogSeverity GetLogMessageType(const VkDebugReportFlagsEXT flags)
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


        VKAPI_ATTR VkBool32 VKAPI_CALL DebugReportCallback(const VkDebugReportFlagsEXT flags,
                                                           [[maybe_unused]] VkDebugReportObjectTypeEXT objectType,
                                                           [[maybe_unused]] uint64_t object, [[maybe_unused]] size_t location,
                                                           [[maybe_unused]] int32_t messageCode,
                                                           [[maybe_unused]] const char* pLayerPrefix, const char* pMessage,
                                                           void* pUserData)
        {
            constexpr static auto ignoredMessages =
                std::array{ "vkCreateShaderModule(): SPIR-V Extension SPV_GOOGLE_hlsl_functionality1", "Device Extension:" };

            const festd::string_view message = pMessage;
            for (auto& msg : ignoredMessages)
            {
                if (message.starts_with(msg))
                    return VK_FALSE;
            }

            const LogSeverity type = GetLogMessageType(flags);
            static_cast<Logger*>(pUserData)->Log(type, "{}", message);

            if (Build::IsDebug() && type == LogSeverity::kError)
                FE_DebugBreak();

            return VK_FALSE;
        }
#endif


        Core::AdapterKind Translate(const VkPhysicalDeviceType src) noexcept
        {
            switch (src)
            {
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                return Core::AdapterKind::kIntegrated;
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                return Core::AdapterKind::kDiscrete;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                return Core::AdapterKind::kVirtual;
            case VK_PHYSICAL_DEVICE_TYPE_CPU:
                return Core::AdapterKind::kCPU;
            default:
                return Core::AdapterKind::kNone;
            }
        }
    } // namespace


    DeviceFactory::~DeviceFactory()
    {
        vkDestroyDebugReportCallbackEXT(m_instance, m_debug, VK_NULL_HANDLE);
        vkDestroyInstance(m_instance, VK_NULL_HANDLE);
        m_logger->LogInfo("Vulkan instance was destroyed");
    }


    Core::ResultCode DeviceFactory::CreateDevice(const Env::Name adapterName)
    {
        FE_PROFILER_ZONE();

        for (uint32_t adapterIndex = 0; adapterIndex < m_adapters.size(); ++adapterIndex)
        {
            if (m_adapters[adapterIndex].m_name == adapterName)
            {
                Rc device = Env::GetServiceProvider()->ResolveRequired<Core::Device>();
                ImplCast(device.Get())->Init(m_nativeAdapters[adapterIndex]);
                return Core::ResultCode::kSuccess;
            }
        }

        return Core::ResultCode::kUnknownError;
    }


    void DeviceFactory::RegisterServices(DI::ServiceRegistryBuilder& builder)
    {
        FE_PROFILER_ZONE();

        // public singletons
        builder.Bind<Core::Device>().To<Device>().InSingletonScope();
        builder.Bind<Core::ResourcePool>().To<ResourcePool>().InSingletonScope();
        builder.Bind<Core::AsyncCopyQueue>().To<AsyncCopyQueue>().InSingletonScope();
        builder.Bind<Core::PipelineFactory>().To<PipelineFactory>().InSingletonScope();
        builder.Bind<Core::GeometryPool>().To<GeometryPool>().InSingletonScope();
        builder.Bind<Core::ShaderLibrary>().To<ShaderLibrary>().InSingletonScope();

        // private singletons
        builder.Bind<Core::ShaderSourceCache>().ToSelf().InSingletonScope();
        builder.Bind<Core::ShaderCompiler>().To<Core::ShaderCompilerDXC>().InSingletonScope();
        builder.Bind<Common::FrameGraphResourcePool>().ToSelf().InSingletonScope();
        builder.Bind<DescriptorAllocator>().ToSelf().InSingletonScope();

        // TODO: remove these transient services
        builder.Bind<Core::Fence>()
            .ToFunc([](DI::IServiceProvider* serviceProvider, Memory::RefCountedObjectBase** result) {
                *result = Fence::Create(serviceProvider->ResolveRequired<Core::Device>());
                return DI::ResultCode::kSuccess;
            })
            .InTransientScope();
        builder.Bind<Core::FrameGraph>().To<FrameGraph>().InTransientScope();
        builder.Bind<Core::Viewport>().To<Viewport>().InTransientScope();
    }


    DeviceFactory::DeviceFactory(Env::Configuration* config, Logger* logger)
        : m_logger(logger)
    {
        FE_PROFILER_ZONE();

        VerifyVulkan(volkInitialize());

        uint32_t layerCount;
        VerifyVulkan(vkEnumerateInstanceLayerProperties(&layerCount, nullptr));

        festd::vector layers(layerCount, VkLayerProperties{});
        VerifyVulkan(vkEnumerateInstanceLayerProperties(&layerCount, layers.data()));
        for (const char* layer : kRequiredInstanceLayers)
        {
            const festd::string_view layerSlice{ layer };
            const bool found = eastl::any_of(layers.begin(), layers.end(), [&](const VkLayerProperties& props) {
                return layerSlice == props.layerName;
            });
            FE_AssertMsg(found, "Vulkan instance layer {} was not found", layerSlice);
        }

        uint32_t extensionCount;
        VerifyVulkan(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr));

        festd::vector extensions(extensionCount, VkExtensionProperties{});
        VerifyVulkan(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data()));
        for (const char* ext : kRequiredInstanceExtensions)
        {
            const festd::string_view extSlice{ ext };
            const bool found = eastl::any_of(extensions.begin(), extensions.end(), [&](const VkExtensionProperties& props) {
                return extSlice == props.extensionName;
            });
            FE_AssertMsg(found, "Vulkan instance extension {} was not found", extSlice);
        }

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.apiVersion = VK_API_VERSION_1_3;
        appInfo.pEngineName = "Ferrum3D";
        appInfo.pApplicationName = config->GetName("ApplicationName", Env::Name::kEmpty).c_str();

        VkInstanceCreateInfo instanceCI{};
        instanceCI.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceCI.pApplicationInfo = &appInfo;
        instanceCI.enabledLayerCount = static_cast<uint32_t>(kRequiredInstanceLayers.size());
        instanceCI.ppEnabledLayerNames = kRequiredInstanceLayers.data();
        instanceCI.enabledExtensionCount = static_cast<uint32_t>(kRequiredInstanceExtensions.size());
        instanceCI.ppEnabledExtensionNames = kRequiredInstanceExtensions.data();

        VerifyVulkan(vkCreateInstance(&instanceCI, VK_NULL_HANDLE, &m_instance));
        volkLoadInstance(m_instance);

#if FE_DEVELOPMENT
        if (config->GetInt("Graphics/DebugRuntime", 1) != 0)
        {
            VkDebugReportCallbackCreateInfoEXT debugCI{};
            debugCI.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
            debugCI.flags |= VK_DEBUG_REPORT_WARNING_BIT_EXT;
            debugCI.flags |= VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
            debugCI.flags |= VK_DEBUG_REPORT_ERROR_BIT_EXT;
            debugCI.flags |= VK_DEBUG_REPORT_DEBUG_BIT_EXT;
            debugCI.pfnCallback = &DebugReportCallback;
            debugCI.pUserData = m_logger.Get();
            VerifyVulkan(vkCreateDebugReportCallbackEXT(m_instance, &debugCI, VK_NULL_HANDLE, &m_debug));
        }
#endif

        m_logger->LogInfo("Vulkan instance created successfully");

        uint32_t adapterCount;
        VerifyVulkan(vkEnumeratePhysicalDevices(m_instance, &adapterCount, nullptr));

        m_nativeAdapters.resize(adapterCount, VkPhysicalDevice{});
        VerifyVulkan(vkEnumeratePhysicalDevices(m_instance, &adapterCount, m_nativeAdapters.data()));
        for (const VkPhysicalDevice physicalDevice : m_nativeAdapters)
        {
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(physicalDevice, &props);
            m_logger->LogInfo("Found Vulkan-compatible GPU: {}", festd::string_view(props.deviceName));

            Core::AdapterInfo& info = m_adapters.emplace_back();
            info.m_kind = Translate(props.deviceType);
            info.m_name = Env::Name{ props.deviceName };
        }
    }


    festd::span<const Core::AdapterInfo> DeviceFactory::EnumerateAdapters() const
    {
        return m_adapters;
    }
} // namespace FE::Graphics::Vulkan
