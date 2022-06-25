#include <OsGPU/Adapter/VKAdapter.h>
#include <OsGPU/Device/VKDevice.h>
#include <OsGPU/Instance/VKInstance.h>

namespace FE::Osmium
{
    VKAdapter::VKAdapter(VKInstance& instance, const vk::PhysicalDevice& vkAdapter)
        : m_VkAdapter(vkAdapter)
        , Prop(vkAdapter.getProperties())
        , m_Instance(&instance)
    {
        m_Desc.Name = Prop.deviceName.data();
        m_Desc.Type = VKConvert(Prop.deviceType);
    }

    AdapterDesc& VKAdapter::GetDesc()
    {
        return m_Desc;
    }

    vk::PhysicalDevice& VKAdapter::GetNativeAdapter()
    {
        return m_VkAdapter;
    }

    Shared<IDevice> VKAdapter::CreateDevice()
    {
        return static_pointer_cast<IDevice>(MakeShared<VKDevice>(*this));
    }

    IInstance& VKAdapter::GetInstance()
    {
        return *m_Instance;
    }
} // namespace FE::Osmium
