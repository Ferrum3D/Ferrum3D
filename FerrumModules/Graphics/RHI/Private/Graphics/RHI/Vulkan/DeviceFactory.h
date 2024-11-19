#pragma once
#include <FeCore/Modules/Configuration.h>
#include <Graphics/RHI/DeviceFactory.h>
#include <Graphics/RHI/Vulkan/Common/Config.h>

namespace FE::DI
{
    class ServiceRegistryBuilder;
}

namespace FE::Graphics::Vulkan
{
    struct Device;

    struct DeviceFactory final : public RHI::DeviceFactory
    {
        FE_RTTI_Class(DeviceFactory, "4247535C-3E97-42E7-A869-1DC542AFBF25");

        DeviceFactory(Env::Configuration* config, Logger* logger);
        ~DeviceFactory() override;

        [[nodiscard]] VkInstance GetNative() const
        {
            return m_instance;
        }

        RHI::ResultCode CreateDevice(Env::Name adapterName) override;

        void RegisterServices(DI::ServiceRegistryBuilder& builder);

        [[nodiscard]] festd::span<const RHI::AdapterInfo> EnumerateAdapters() const override;

    private:
        VkInstance m_instance = VK_NULL_HANDLE;
        VkDebugReportCallbackEXT m_debug = VK_NULL_HANDLE;

        festd::vector<RHI::AdapterInfo> m_adapters;
        festd::vector<VkPhysicalDevice> m_nativeAdapters;
        Rc<Logger> m_logger;
    };

    FE_ENABLE_NATIVE_CAST(DeviceFactory);
} // namespace FE::Graphics::Vulkan
