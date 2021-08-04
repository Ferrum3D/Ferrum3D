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

    class VKAdapter : public IAdapter
    {
        AdapterDesc m_Desc;
        vk::PhysicalDevice m_VkAdapter;
        vk::PhysicalDeviceProperties m_Prop;

    public:
        VKAdapter(const vk::PhysicalDevice& vkAdapter);

        virtual AdapterDesc& GetDesc() override;
        vk::PhysicalDevice* GetNativeAdapter();
        virtual RefCountPtr<IDevice> CreateDevice() override;
    };
} // namespace FE::GPU
