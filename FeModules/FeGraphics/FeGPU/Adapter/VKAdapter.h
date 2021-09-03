#pragma once
#include <FeGPU/Adapter/IAdapter.h>
#include <FeGPU/Instance/VKInstance.h>

namespace FE::GPU
{
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
        virtual AdapterDesc& GetDesc() override;
        virtual Shared<IDevice> CreateDevice() override;
        virtual IInstance& GetInstance() override;
    };
} // namespace FE::GPU
