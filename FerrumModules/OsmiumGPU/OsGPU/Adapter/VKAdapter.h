#pragma once
#include <OsGPU/Adapter/IAdapter.h>
#include <OsGPU/Common/VKConfig.h>

namespace FE::Osmium
{
    class VKInstance;

    inline AdapterType VKConvert(VkPhysicalDeviceType src) noexcept
    {
        switch (src)
        {
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            return AdapterType::Integrated;
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            return AdapterType::Discrete;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            return AdapterType::Virtual;
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            return AdapterType::CPU;
        default:
            return AdapterType::None;
        }
    }

    class VKAdapter : public Object<IAdapter>
    {
        AdapterDesc m_Desc;
        VKInstance* m_Instance;
        VkPhysicalDevice m_VkAdapter;

    public:
        FE_CLASS_RTTI(VKAdapter, "4054CC7E-C6EB-4A43-B326-E85C32BED38C");

        VkPhysicalDeviceProperties Properties;

        VKAdapter(VKInstance& instance, VkPhysicalDevice vkAdapter);
        ~VKAdapter() override = default;

        VkPhysicalDevice GetNativeAdapter();
        AdapterDesc& GetDesc() override;
        Rc<IDevice> CreateDevice() override;
        IInstance& GetInstance() override;
    };
} // namespace FE::Osmium
