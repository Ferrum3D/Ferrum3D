#include <FeGPU/Adapter/VKAdapter.h>
#include <FeGPU/Device/VKDevice.h>

namespace FE::GPU
{
    VKAdapter::VKAdapter(VKInstance& instance, const vk::PhysicalDevice& vkAdapter)
        : m_VkAdapter(vkAdapter)
        , m_Prop(vkAdapter.getProperties())
        , m_Instance(&instance)
    {
        m_Desc.Name = m_Prop.deviceName.data();
        m_Desc.Type = VKConvert(m_Prop.deviceType);
    }

    AdapterDesc& VKAdapter::GetDesc()
    {
        return m_Desc;
    }

    vk::PhysicalDevice& VKAdapter::GetNativeAdapter()
    {
        return m_VkAdapter;
    }

    RefCountPtr<IDevice> VKAdapter::CreateDevice()
    {
        return StaticPtrCast<IDevice>(MakeShared<VKDevice>(*this));
    }

    IInstance& VKAdapter::GetInstance()
    {
        return *m_Instance;
    }
} // namespace FE::GPU
