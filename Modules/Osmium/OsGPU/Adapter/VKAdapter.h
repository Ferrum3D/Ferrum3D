#pragma once
#include <OsGPU/Adapter/IAdapter.h>
#include <OsGPU/Common/VKConfig.h>

namespace FE::GPU
{
    class VKInstance;

    inline AdapterType VKConvert(const vk::PhysicalDeviceType& src) noexcept
    {
        switch (src)
        {
        case vk::PhysicalDeviceType::eIntegratedGpu:
            return AdapterType::Integrated;
        case vk::PhysicalDeviceType::eDiscreteGpu:
            return AdapterType::Discrete;
        case vk::PhysicalDeviceType::eVirtualGpu:
            return AdapterType::Virtual;
        case vk::PhysicalDeviceType::eCpu:
            return AdapterType::CPU;
        default:
            return AdapterType::None;
        }
    }

    class VKAdapter : public Object<IAdapter>
    {
        AdapterDesc m_Desc;
        VKInstance* m_Instance;
        vk::PhysicalDevice m_VkAdapter;
        vk::PhysicalDeviceProperties m_Prop;

    public:
        FE_CLASS_RTTI(VKAdapter, "4054CC7E-C6EB-4A43-B326-E85C32BED38C");

        VKAdapter(VKInstance& instance, const vk::PhysicalDevice& vkAdapter);

        vk::PhysicalDevice& GetNativeAdapter();
        AdapterDesc& GetDesc() override;
        Shared<IDevice> CreateDevice() override;
        IInstance& GetInstance() override;
    };
} // namespace FE::GPU
