#include <Graphics/Core/DeviceFactory.h>
#include <Graphics/Core/Vulkan/DeviceFactory.h>

namespace FE::Graphics::Core
{
    static Rc<DeviceFactory> GDeviceFactory;


    void DeviceFactory::Init(const GraphicsAPI api)
    {
        FE_Assert(GDeviceFactory == nullptr, "DeviceFactory already initialized");
        switch (api)
        {
        default:
        case GraphicsAPI::kNone:
            FE_DebugBreak();
            [[fallthrough]];

        case GraphicsAPI::kVulkan:
            GDeviceFactory = Memory::DefaultNew<Vulkan::DeviceFactory>();
            break;
        }
    }


    void DeviceFactory::Shutdown()
    {
        FE_Assert(GDeviceFactory != nullptr, "DeviceFactory not initialized");
        GDeviceFactory.Reset();
    }


    DeviceFactory& DeviceFactory::Get()
    {
        return *GDeviceFactory;
    }
} // namespace FE::Graphics::Core
