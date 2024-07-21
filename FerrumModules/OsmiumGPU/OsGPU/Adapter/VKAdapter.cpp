#include <OsGPU/Adapter/VKAdapter.h>
#include <OsGPU/Device/VKDevice.h>
#include <OsGPU/Instance/VKInstance.h>

namespace FE::Osmium
{
    VKAdapter::VKAdapter(VKInstance& instance, VkPhysicalDevice vkAdapter)
        : m_VkAdapter(vkAdapter)
        , m_Instance(&instance)
    {
        vkGetPhysicalDeviceProperties(m_VkAdapter, &Properties);
        m_Desc.Name = Properties.deviceName;
        m_Desc.Type = VKConvert(Properties.deviceType);
    }

    AdapterDesc& VKAdapter::GetDesc()
    {
        return m_Desc;
    }

    VkPhysicalDevice VKAdapter::GetNativeAdapter()
    {
        return m_VkAdapter;
    }

    Rc<IDevice> VKAdapter::CreateDevice()
    {
        return Rc<VKDevice>::DefaultNew(*this);
    }

    IInstance& VKAdapter::GetInstance()
    {
        return *m_Instance;
    }
} // namespace FE::Osmium
