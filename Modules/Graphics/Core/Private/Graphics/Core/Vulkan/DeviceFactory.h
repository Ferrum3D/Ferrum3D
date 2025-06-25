#pragma once
#include <FeCore/Modules/Configuration.h>
#include <Graphics/Core/DeviceFactory.h>
#include <Graphics/Core/Vulkan/Base/Config.h>

namespace FE::DI
{
    struct ServiceRegistryBuilder;
}

namespace FE::Graphics::Vulkan
{
    struct Device;

    struct DeviceFactory final : public Core::DeviceFactory
    {
        FE_RTTI_Class(DeviceFactory, "4247535C-3E97-42E7-A869-1DC542AFBF25");

        DeviceFactory(Env::Configuration* config, Logger* logger);
        ~DeviceFactory() override;

        [[nodiscard]] VkInstance GetNative() const
        {
            return m_instance;
        }

        Core::ResultCode CreateDevice(Env::Name adapterName) override;

        void RegisterServices(const DI::ServiceRegistryBuilder& builder);

        [[nodiscard]] festd::span<const Core::AdapterInfo> EnumerateAdapters() const override;

    private:
        VkInstance m_instance = VK_NULL_HANDLE;
        VkDebugReportCallbackEXT m_debug = VK_NULL_HANDLE;

        festd::inline_vector<Core::AdapterInfo> m_adapters;
        festd::inline_vector<VkPhysicalDevice> m_nativeAdapters;
        Rc<Logger> m_logger;
    };

    FE_ENABLE_NATIVE_CAST(DeviceFactory);
} // namespace FE::Graphics::Vulkan
