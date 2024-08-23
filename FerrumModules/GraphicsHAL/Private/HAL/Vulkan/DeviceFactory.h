#pragma once
#include <FeCore/Modules/Configuration.h>
#include <HAL/DeviceFactory.h>
#include <HAL/Vulkan/Common/Config.h>

namespace FE::DI
{
    class ServiceRegistryBuilder;
}

namespace FE::Graphics::Vulkan
{
    class Device;

    class DeviceFactory final : public HAL::DeviceFactory
    {
        VkInstance m_Instance = VK_NULL_HANDLE;
        VkDebugReportCallbackEXT m_Debug = VK_NULL_HANDLE;

        festd::vector<HAL::AdapterInfo> m_Adapters;
        festd::vector<VkPhysicalDevice> m_NativeAdapters;
        Rc<Debug::IConsoleLogger> m_pLogger;

    public:
        FE_RTTI_Class(DeviceFactory, "4247535C-3E97-42E7-A869-1DC542AFBF25");

        DeviceFactory(Env::Configuration* pConfig, Debug::IConsoleLogger* pLogger);
        ~DeviceFactory() override;

        [[nodiscard]] inline VkInstance GetNative() const
        {
            return m_Instance;
        }

        HAL::ResultCode CreateDevice(Env::Name adapterName) override;

        void RegisterServices(DI::ServiceRegistryBuilder& builder);

        [[nodiscard]] festd::span<const HAL::AdapterInfo> EnumerateAdapters() const override;
    };

    FE_ENABLE_IMPL_CAST(DeviceFactory);
} // namespace FE::Graphics::Vulkan
